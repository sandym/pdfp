#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdexcept>
#include "su/base/platform.h"
#include "su/json/json.h"
#include "su/files/filepath.h"
#include "su/strings/str_ext.h"
#include <array>
#include <stack>
#include <unordered_map>

#include "pdfp_dumper.h"

#ifdef PDFP_WANT_QUARTZ
#	include "quartz/quartz.h"
#endif
#ifdef PDFP_WANT_POPPLER
#	include "poppler/poppler.h"
#endif
#ifdef PDFP_WANT_APDFL
#	include "apdfl/apdfl.h"
#endif
#ifdef PDFP_WANT_OPENPDF
#	include "openpdf/openpdf.h"
#endif

#include "dumper.h"

#include <malloc/malloc.h>

//! name of current process
std::string g_argv0;

int usage()
{
	std::cout << "pdf_tests run_tests [driver list] [file or folder path]\n";
	std::cout << "or\n";
	std::cout << "pdf_tests dump [driver] [file path] {-o output_file}\n\n";
	std::cout << "supported drivers:\n";
	std::cout << "  pdfp\n";
#ifdef PDFP_WANT_QUARTZ
	std::cout << "  quartz\n";
#endif
#ifdef PDFP_WANT_POPPLER
	std::cout << "  poppler\n";
#endif
#ifdef PDFP_WANT_APDFL
	std::cout << "  apdfl\n";
#endif
#ifdef PDFP_WANT_OPENPDF
	std::cout << "  openpdf\n";
#endif
	return -1;
}

uint64_t getPeakMemUsed()
{
	auto zone = malloc_default_zone();
	malloc_statistics_t stats{};
	zone->introspect->statistics( zone, &stats );
	return stats.max_size_in_use / 1024;
}

int dump( const std::string_view &driver,
          const std::string &file,
          const std::string &outputFile )
{
	std::unique_ptr<std::ofstream> output_file;
	std::ostream *output_stream = &std::cout;
	if ( not outputFile.empty() )
	{
		output_file = std::make_unique<std::ofstream>( outputFile );
		output_stream = output_file.get();
	}
#ifdef PDFP_WANT_APDFL
	if ( driver == "apdfl" )
		apdfl_init();
#endif
#ifdef PDFP_WANT_OPENPDF
	if ( driver == "openpdf" )
		openpdf_init();
#endif

	int ret = 0;
	auto t = std::chrono::high_resolution_clock::now();
	//for ( int i = 0; i < 5; ++i )
	{
		try
		{
			if ( driver == "pdfp" )
			{
				auto doc = std::make_unique<pdfp::Document>();
				doc->open( file );
				doc->preload();
				dumpDocument( doc.get(), *output_stream );
			}
#ifdef PDFP_WANT_QUARTZ
			else if ( driver == "quartz" )
			{
				auto doc = quartz_open( file );
				dumpDocument( doc.get(), *output_stream );
			}
#endif
#ifdef PDFP_WANT_POPPLER
			else if ( driver == "poppler" )
			{
				auto doc = poppler_open( file );
				dumpDocument( doc.get(), *output_stream );
			}
#endif
#ifdef PDFP_WANT_APDFL
			else if ( driver == "apdfl" )
			{
				auto doc = apdfl_open( file );
				dumpDocument( doc.get(), *output_stream );
			}
#endif
#ifdef PDFP_WANT_OPENPDF
			else if ( driver == "openpdf" )
			{
				auto doc = openpdf_open( file );
				dumpDocument( doc.get(), *output_stream );
			}
#endif
			// some tests driver
			else if ( driver == "abort" )
			{
				abort();
			}
			else if ( driver == "exit0" )
			{
				exit( 0 );
			}
			else if ( driver == "exit1" )
			{
				exit( 1 );
			}
		}
		catch ( std::exception &ex )
		{
			std::cout << ex.what() << std::endl;
			*output_stream << "    " << ex.what() << "\n";
			ret = -1;
		}
	}
	auto e = std::chrono::duration_cast<std::chrono::microseconds>(
	             std::chrono::high_resolution_clock::now() - t )
	             .count();
	std::cout << "time: " << e << std::endl;
	std::cout << "mem: " << getPeakMemUsed() << std::endl;
	return ret;
}

std::vector<su::filepath> getAllPDFs( const su::filepath &input )
{
	std::vector<su::filepath> result;
	if ( input.isFolder() )
	{
		std::stack<su::filepath> folders;
		folders.push( input );
		while ( not folders.empty() )
		{
			auto current = folders.top();
			folders.pop();
			auto all = current.folderContent();
			for ( auto &it : all )
			{
				if ( su::tolower( it.extension() ) == "pdf" )
					result.push_back( it );
				else if ( it.isFolder() )
					folders.push( it );
			}
		}
		std::sort( result.begin(),
		           result.end(),
		           []( const su::filepath &lhs, const su::filepath &rhs ) {
			           return lhs.file_size() < rhs.file_size();
		           } );
	}
	else if ( su::tolower( input.extension() ) == "pdf" )
		result.push_back( input );
	return result;
}

struct DriverStats
{
	int nbOfCrashes{0};
	int nbOfFails{0};
};
struct FileDriverStats
{
	bool crashed{ false };
	bool cannotParse{ false };
	bool sameAsPDFP{ false };
	uint64_t t{ 0 };
	uint64_t memused{ 0 };

	FileDriverStats() = default;
	FileDriverStats( const su::Json &i_json );
	su::Json to_json() const;
};
struct FileStats
{
	int numberOfTimeTested{0};
	int numberOfTimeAgreedWithAll{0};

	std::string fileChecksum;
	std::unordered_map<std::string,FileDriverStats> driverStats;

	bool canBeParseWith( const std::string &driver ) const;
	bool lastTestWasAllGood( const std::vector<std::string> &drivers ) const;
	bool lastTestWasOnlySlow( const std::vector<std::string> &drivers ) const;

	FileStats() = default;
	FileStats( const su::Json &i_json );
	su::Json to_json() const;
};

FileDriverStats::FileDriverStats( const su::Json &i_json )
{
	crashed = i_json["crashed"].bool_value();
	cannotParse = i_json["cannotParse"].bool_value();
	sameAsPDFP = i_json["sameAsPDFP"].bool_value();
	t = i_json["t"].int64_value();
	memused = i_json["memused"].int64_value();

}
su::Json FileDriverStats::to_json() const
{
	su::Json::object json;
	json["crashed"] = crashed;
	json["cannotParse"] = cannotParse;
	json["sameAsPDFP"] = sameAsPDFP;
	json["t"] = t;
	json["memused"] = memused;
	return json;
}
FileStats::FileStats( const su::Json &i_json )
{
	numberOfTimeTested = i_json["numberOfTimeTested"].int_value();
	numberOfTimeAgreedWithAll = i_json["numberOfTimeAgreedWithAll"].int_value();
	for ( auto &it : i_json["driverStats"].object_items() )
		driverStats[it.first] = FileDriverStats( it.second );
	fileChecksum = i_json["fileChecksum"].string_value();
}
su::Json FileStats::to_json() const
{
	return {{{"numberOfTimeTested", numberOfTimeTested},
	         {"numberOfTimeAgreedWithAll", numberOfTimeAgreedWithAll},
	         {"fileChecksum", fileChecksum},
	         {"driverStats", driverStats}}};
}
bool FileStats::canBeParseWith( const std::string &driver ) const
{
	auto it = driverStats.find( driver );
	if ( it != driverStats.end() )
		return not it->second.cannotParse;
	return true;
}
bool FileStats::lastTestWasAllGood( const std::vector<std::string> &drivers ) const
{
	auto pdfp = driverStats.find( "pdfp" );
	if ( pdfp == driverStats.end() )
		return false;
	
	// if any crashed or any was faster
	// or any used less mem or any disagree
	for ( auto &it : driverStats )
	{
		if ( std::find( drivers.begin(), drivers.end(), it.first ) == drivers.end() )
			continue;
		
		if ( it.second.cannotParse )
			continue;
		if ( it.second.crashed )
			return false;
		if ( it.first != "pdfp" )
		{
			if ( not it.second.sameAsPDFP )
				return false;
			if ( it.second.t < pdfp->second.t )
				return false;
			if ( it.second.memused < pdfp->second.memused )
				return false;
		}
	}
	return true;
}
bool FileStats::lastTestWasOnlySlow( const std::vector<std::string> &drivers ) const
{
	auto pdfp = driverStats.find( "pdfp" );
	if ( pdfp == driverStats.end() )
		return false;
	
	// if any crashed or any disagree
	for ( auto &it : driverStats )
	{
		if ( std::find( drivers.begin(), drivers.end(), it.first ) == drivers.end() )
			continue;

		if ( it.second.cannotParse )
			continue;
		if ( it.second.crashed )
			return false;
		if ( it.first != "pdfp" )
		{
			if ( not it.second.sameAsPDFP )
				return false;
		}
	}
	return true;
}

void saveFileStats( const su::filepath &i_pdf, const FileStats &stats )
{
	auto jsonFile = i_pdf;
	jsonFile.setExtension( "json" );
	auto txt = stats.to_json().dump();
	std::ofstream ostr;
	if ( jsonFile.fsopen( ostr ) )
		ostr << txt << "\n";
}

FileStats loadFileStats( const su::filepath &i_pdf )
{
	auto jsonFile = i_pdf;
	jsonFile.setExtension( "json" );
	std::ifstream str;
	if ( jsonFile.fsopen( str ) )
	{
		std::string txt( ( std::istreambuf_iterator<char>( str ) ),
		                 ( std::istreambuf_iterator<char>() ) );
		str.close();
		std::string err;
		auto json = su::Json::parse( txt, err );
		if ( err.empty() )
		{
			FileStats stats{json};
			if ( not stats.fileChecksum.empty() )
				return stats;
		}
	}
	// new file
	FileStats stats;
	// file checkum
	std::ifstream pdfstr;
	if ( i_pdf.fsopen( pdfstr ) )
	{
		pdfp::MD5_CTX c;
		pdfp::MD5_Init( &c );

		size_t l = 0;
		while ( pdfstr )
		{
			std::array<uint8_t, 4096> buffer;
			memset( buffer.data(), 0, 4096 );
			pdfstr.read( (char *)buffer.data(), 4096 );
			auto len = pdfstr.gcount();
			if ( len <= 0 )
				break;

			l += len;

			pdfp::MD5_Update( &c, buffer.data(), len );
		}

		unsigned char md[pdfp::MD5_DIGEST_LENGTH];
		pdfp::MD5_Final( md, &c );
		for ( int i = 0; i < pdfp::MD5_DIGEST_LENGTH; ++i )
		{
			int hi = ( md[i] & 0xF0 ) >> 4;
			int lo = ( md[i] & 0x0F );
			stats.fileChecksum.push_back( hi < 10 ? hi + '0' : 'A' + hi - 10 );
			stats.fileChecksum.push_back( lo < 10 ? lo + '0' : 'A' + lo - 10 );
		}
		pdfstr.close();
	}
	auto txt = stats.to_json().dump();
	std::ofstream ostr;
	if ( jsonFile.fsopen( ostr ) )
		ostr << txt << "\n";
	return stats;
}

void appendFromFd( int fd, std::string &s )
{
	for ( ;; )
	{
		char buffer[4096];
		auto l = read( fd, buffer, 4096 );
		if ( l <= 0 )
			break;
		s.append( buffer, l );
	}
}
struct TestResult
{
	int exit{-1};
	bool crash{false};
	uint64_t t{0};
	uint64_t m{0};
	su::filepath output;
	std::string diff;

	bool unableToParse{false};
};
TestResult test_driver( const std::string &driver, const su::filepath &pdfFile )
{
	TestResult result;
	result.output = su::filepath( "/tmp" );
	result.output.add( driver + "_output.txt" );
	result.output.unlink();

	// pipe
	int child2ParentPipe[2];
	int err = pipe( child2ParentPipe );
	assert( err == 0 );

//	std::cout << g_argv0 <<
//				" dump " << driver << " \"" <<
//				pdfFile.path() << "\" -o " << result.output.path() << "\n";

	// fork
	auto pid = fork();
	assert( pid >= 0 );

	if ( pid == 0 )
	{
		close( child2ParentPipe[0] );
		if ( child2ParentPipe[1] != STDOUT_FILENO )
		{
			if ( dup2( child2ParentPipe[1], STDOUT_FILENO ) != STDOUT_FILENO )
			{
				std::cerr << "dup2 error to stdout" << std::endl;
				exit( -1 );
			}
			close( child2ParentPipe[1] );
		}

		// exec
		execl( g_argv0.c_str(),
		       g_argv0.c_str(),
		       "dump",
		       driver.c_str(),
		       pdfFile.path().c_str(),
		       "-o",
		       result.output.path().c_str(),
		       (char *)0 );
		std::cerr << "fatal error" << std::endl;
		exit( -1 );
	}

	close( child2ParentPipe[1] );

	std::string output;
	int stat_loc = 0;
	while ( waitpid( pid, &stat_loc, WNOHANG ) != pid )
	{
		appendFromFd( child2ParentPipe[0], output );
		// sleep(1);
	}
	appendFromFd( child2ParentPipe[0], output );
	close( child2ParentPipe[0] );

	if ( WIFEXITED( stat_loc ) )
	{
		result.exit = WEXITSTATUS( stat_loc );

		if ( result.exit == 0 )
		{
			// get the time and mem
			auto lines = su::split( output, '\n' );
			if ( lines.size() >= 2 )
			{
				if ( su::starts_with( lines[lines.size()-2], "time: " ) )
				{
					result.t = std::stoull( lines[lines.size()-2].substr( 6 ) );
				}
				if ( su::starts_with( lines[lines.size()-1], "mem: " ) )
				{
					result.m = std::stoull( lines[lines.size()-1].substr( 5 ) );
				}
			}
		}
	}
	else if ( WIFSIGNALED( stat_loc ) )
	{
		// auto sig = WTERMSIG(stat_loc);
		result.crash = true;
	}

	return result;
}

std::string captureDiff( const su::filepath &a, const su::filepath &b )
{
	// pipe
	int child2ParentPipe[2];
	int err = pipe( child2ParentPipe );
	assert( err == 0 );

	// fork
	auto pid = fork();
	assert( pid >= 0 );

	if ( pid == 0 )
	{
		close( child2ParentPipe[0] );
		if ( child2ParentPipe[1] != STDOUT_FILENO )
		{
			if ( dup2( child2ParentPipe[1], STDOUT_FILENO ) != STDOUT_FILENO )
			{
				std::cerr << "dup2 error to stdout" << std::endl;
				exit( -1 );
			}
			close( child2ParentPipe[1] );
		}

		// exec
		execl( "/usr/bin/diff",
		       "/usr/bin/diff",
		       a.path().c_str(),
		       b.path().c_str(),
		       (char *)0 );
		std::cerr << "fatal error" << std::endl;
		exit( -1 );
	}

	close( child2ParentPipe[1] );

	std::string output;
	int stat_loc = 0;
	while ( waitpid( pid, &stat_loc, WNOHANG ) != pid )
	{
		appendFromFd( child2ParentPipe[0], output );
		// sleep(1);
	}
	appendFromFd( child2ParentPipe[0], output );
	close( child2ParentPipe[0] );

	if ( WIFEXITED( stat_loc ) )
	{
	}
	else if ( WIFSIGNALED( stat_loc ) )
	{
		auto sig = WTERMSIG( stat_loc );
		output = "crash: " + std::to_string( sig );
	}
	return output;
}

std::string html( const std::string &s )
{
	std::string result;
	result.reserve( s.size() );
	for ( auto c : s )
	{
		switch ( c )
		{
			case '\n':
				result += "<br/>\n";
				break;
			case '&':
				result += "&amp;";
				break;
			case '<':
				result += "&lt;";
				break;
			default:
				result.push_back( c );
				break;
		}
	}
	return result;
}
std::string cell( const std::string &s,
                  bool error = false,
                  const std::string &c = {},
                  const std::string &tooltip = {} )
{
	std::string r = R"(<td align="center")";
	if ( error )
		r += R"( class="error")";
	r += ">";
	if ( not tooltip.empty() )
		r += R"(<div class="tooltip">)";

	if ( not c.empty() )
		r += R"(<font color=")" + c + R"(">)";
	r += s;
	if ( not c.empty() )
		r += "</font>";
	if ( not tooltip.empty() )
	{
		r += R"(<span class="tooltiptext">)" + html( tooltip ) + R"(</span>)";
		r += R"(</div>)";
	}
	r += "</td>";
	return r;
};

std::string diffPerf( uint64_t referenceTime, uint64_t otherTime,
						uint64_t referenceMem, uint64_t otherMem )
{
	std::string result;
	if ( referenceTime != otherTime and referenceTime != 0 )
	{
		int perc = std::ceil(
		    ( ( double( referenceTime ) - double( otherTime ) ) / double( referenceTime ) ) *
	    	100 );
		if ( perc > 0 )
			result += "faster : " + std::to_string( perc ) + "%";
		else if ( perc < 0 )
			result += "slower:  " + std::to_string( -perc ) + "%";
	}
	if ( referenceMem != otherMem and referenceMem != 0 )
	{
		if ( not result.empty() )
			result += "\n";
		if ( otherMem > referenceMem )
			result += "uses more memory : ";
		else
			result += "uses less memory : ";
		result += std::to_string( otherMem ) + "/" + std::to_string( referenceMem );
	}
	return result;
}

std::string limitLines( const std::string &s, int nbOfLines )
{
	std::string::size_type p = 0;
	while ( p != std::string::npos and nbOfLines > 0 )
	{
		p = s.find( '\n', p + 1 );
		--nbOfLines;
	}
	if ( p != std::string::npos )
		return s.substr( 0, p );
	return s;
}
void run_tests( const su::filepath &input,
                const std::vector<std::string> &drivers,
                int skip )
{
	// 1- get all pdf files
	auto allFiles = getAllPDFs( input );

	std::unordered_map<std::string, DriverStats> driverStats;
	for ( auto &it : drivers )
		driverStats[it] = {};

	std::ofstream ostr( "pdfpTestResults.html" );
	ostr << R"(<?xml version="1.0" encoding="UTF-8"?><html>)";
	ostr << "<head>\n";
	ostr << R"(<style>
.tooltip {
    position: relative;
    display: inline-block;
    border-bottom: 1px dotted black;
}

.tooltip .tooltiptext {
    visibility: hidden;
    width: 600px;
    background-color: gray;
    color: #fff;
    text-align: left;
    border-radius: 6px;
    padding: 5px 0;

    /* Position the tooltip */
    position: absolute;
    z-index: 1;
}

.tooltip:hover .tooltiptext {
    visibility: visible;
}

tr.odd { background-color: #EDF3FE; }
tr.headerRow {
	background-color: #8BBBE8;
	font-weight: bold;
}
td.error { background-color: #FF9999; }
</style>
)";
	ostr << "<title>pdfp Test Results</title>\n";
	ostr << "</head>\n";
	ostr << "<body>\n";
	ostr << "<h3>pdfp Test Results</h3>\n";

	// header row: Index Size FileName pdfp [drivers]
	auto outputHeader = [&]()
	{
		ostr << R"(<tr class="headerRow">)";
		ostr << R"(<td align="left">Index</td>)";
		ostr << R"(<td align="left">Size</td>)";
		ostr << R"(<td align="left">File</td>)";
		for ( auto &it : drivers )
			ostr << R"(<td align="center" width="90">)" << it << "</td>";
		ostr << "</tr>";
	};

	ostr << "<table><tbody>\n";
	
	int index = 0;
	for ( auto &pdfFile : allFiles )
	{
		auto fileStats = loadFileStats( pdfFile );
		if ( skip > 0 and fileStats.lastTestWasAllGood( drivers ) )
			continue;
		if ( skip > 1 and fileStats.lastTestWasOnlySlow( drivers ) )
			continue;
		
		if ( (index%25) == 0 )
			outputHeader();
		
		std::cout << "--> " << pdfFile.name() << std::endl;

		if ( index & 1 )
			ostr << R"(<tr class="odd">)";
		else
			ostr << R"(<tr>)";
		++index;

		// index
		ostr << "<td>" << index << "</td>";

		// size
		ostr << "<td>" << pdfFile.file_size() << "</td>";

		// name
		if ( fileStats.numberOfTimeTested == 0 )
		{
			// new file
			ostr << R"(<td><font color="#FF4F0A">)" << pdfFile.stem()
			     << "</font></td>";
		}
		else if ( fileStats.numberOfTimeTested > 100 and
		          fileStats.numberOfTimeAgreedWithAll > 100 )
		{
			// stable file
			ostr << R"(<td><font color="#1FB40A">)" << pdfFile.stem()
			     << "</font></td>";
		}
		else
		{
			ostr << "<td>" << pdfFile.stem() << "</td>";
		}

		// drivers results
		std::unordered_map<std::string, TestResult> testResults;
		for ( auto &it : drivers )
		{
			std::cout << "    " << it << std::endl;
			if ( fileStats.canBeParseWith( it ) )
				testResults[it] = test_driver( it, pdfFile );
			else
			{
				TestResult tr;
				tr.unableToParse = true;
				testResults[it] = tr;
			}
		}
		++fileStats.numberOfTimeTested;

		// diffs
		auto &pdfpr = testResults["pdfp"];
		if ( pdfpr.exit == 0 ) // don't diff if pdfp failed
		{
			for ( auto &it : drivers )
			{
				if ( it != "pdfp" )
				{
					auto &r = testResults[it];
					r.diff = limitLines( captureDiff( pdfpr.output, r.output ), 20 );
				}
			}
		}
		auto referenceTime = pdfpr.exit == 0 ? pdfpr.t : 0;
		auto referenceMem = pdfpr.exit == 0 ? pdfpr.m : 0;

		// output
		int nbOfGoodResult = 0;
		int nbOfUnableToParse = 0;
		for ( auto &it : drivers )
		{
			auto &ds = driverStats[it];
			auto &r = testResults[it];

			fileStats.driverStats[it].sameAsPDFP = r.diff.empty();
			//fileStats.driverStats[it].cannotParse = r.exit != 0;
			fileStats.driverStats[it].crashed = r.crash;
			fileStats.driverStats[it].t = r.t;
			fileStats.driverStats[it].memused = r.m;

			if ( r.unableToParse )
			{
				ostr << cell( "----" );
				++nbOfUnableToParse;
			}
			else if ( r.crash )
			{
				ostr << cell( "Crash", true, "#F00" );
				++ds.nbOfCrashes;
			}
			else if ( r.exit != 0 )
			{
				ostr << cell( "Invalid", true );
				++ds.nbOfFails;
			}
			else if ( not r.diff.empty() )
			{
				ostr << cell( "OK", true, "#0C0", r.diff );
			}
			else if ( referenceTime > r.t or referenceMem > r.m )
			{
				ostr << cell(
				    "OK", false, "#00C", diffPerf( referenceTime, r.t, referenceMem, r.m ) );
			}
			else
			{
				++nbOfGoodResult;
				ostr << cell( "OK", false, "#0C0", diffPerf( referenceTime, r.t, referenceMem, r.m ) );
			}
		}

		if ( nbOfGoodResult == drivers.size() )
			++fileStats.numberOfTimeAgreedWithAll;

		// update filestats
		saveFileStats( pdfFile, fileStats );

		ostr << "</tr>" << std::endl;
	}
	ostr << "</tbody></table>\n<br/><br/>\n";

	// driver stats
	ostr << "<table><tbody>\n";
	ostr << R"(<tr class="headerRow">)";
	ostr << "<td></td>";
	for ( auto &driver : driverStats )
		ostr << "<td>" << driver.first << "</td>";
	ostr << "</tr>\n";
	ostr << "<tr>";
	ostr << "<td>crashes</td>";
	for ( auto &driver : driverStats )
		ostr << "<td>" << driver.second.nbOfCrashes << "</td>";
	ostr << "</tr>\n";
	ostr << "<td>fails</td>";
	for ( auto &driver : driverStats )
		ostr << "<td>" << driver.second.nbOfFails << "</td>";
	ostr << "</tr>\n";
	ostr << "</tbody></table>\n";

	ostr << "</body></html>\n";
}

int main( int argc, char *const argv[] )
{
	if ( argc < 2 )
		return usage();

	if ( strcmp( argv[1], "dump" ) == 0 and argc >= 4 )
	{
		for ( int i = 0; i < argc; ++i )
			std::cout << argv[i] << " ";
		std::cout << std::endl;

		auto driver = argv[2];
		auto file = argv[3];
		std::string output;
		if ( argc == 6 and strcmp( argv[4], "-o" ) == 0 )
			output = argv[5];
		return dump( driver, file, output );
	}
	else if ( strcmp( argv[1], "run_tests" ) == 0 and argc >= 3 )
	{
		auto input = argv[2];
		int skip = 0;
		std::vector<std::string> drivers{"pdfp"};
		for ( int i = 3; i < argc; ++i )
		{
			std::string n( argv[i] );
			if ( n == "-skip" )
				++skip;
			else if ( std::find( drivers.begin(), drivers.end(), n ) ==
			          drivers.end() )
			{
				drivers.push_back( n );
			}
		}
		g_argv0 = argv[0];
		run_tests( su::filepath( input ), drivers, skip );
	}
	else
		return usage();
	return 0;
}
