#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdexcept>
#include "su/base/platform.h"
#include "su/json/json.h"
#include "su/files/filepath.h"
#include "su/strings/str_ext.h"
#include <array>
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

std::string g_process;

int usage()
{
	return -1;
}

uint64_t getMem()
{
	auto zone = malloc_default_zone();
	malloc_statistics_t stats{};
	zone->introspect->statistics( zone, &stats );
	return stats.max_size_in_use / (1024*1024);
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
	std::cout << "mem: " << getMem() << std::endl;
	return ret;
}

std::vector<su::filepath> getAll( const su::filepath &input )
{
	std::vector<su::filepath> result;
	if ( input.isFolder() )
	{
		auto all = input.folderContent();
		for ( auto &it : all )
		{
			if ( su::tolower( it.extension() ) == "pdf" )
				result.push_back( it );
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
	int nbOfCrash{0};
	int nbOfFail{0};
};
struct FileStats
{
	int nbOfTimeTested{0};
	int nbOfTimeCrashedPDFP{0};
	int nbOfTimeAgreedWithAll{0};
	bool lastTestWasAllGood{false};
	bool lastTestWasOnlySlow{false};

	std::string checksum;

	std::unordered_set<std::string> driversThatCannotParseThis;

	bool canBeParseWith( const std::string &driver ) const
	{
		return driversThatCannotParseThis.find( driver ) ==
		       driversThatCannotParseThis.end();
	}

	FileStats() = default;
	FileStats( const su::Json &i_json );
	su::Json to_json() const;
};
FileStats::FileStats( const su::Json &i_json )
{
	nbOfTimeTested = i_json["nbOfTimeTested"].int_value();
	nbOfTimeCrashedPDFP = i_json["nbOfTimeCrashedPDFP"].int_value();
	nbOfTimeAgreedWithAll = i_json["nbOfTimeAgreedWithAll"].int_value();
	lastTestWasAllGood = i_json["lastTestWasAllGood"].bool_value();
	lastTestWasOnlySlow = i_json["lastTestWasOnlySlow"].bool_value();
	for ( auto &it : i_json["driversThatCannotParseThis"].array_items() )
		driversThatCannotParseThis.insert( it.string_value() );
	checksum = i_json["checksum"].string_value();
}
su::Json FileStats::to_json() const
{
	return {{{"nbOfTimeTested", nbOfTimeTested},
	         {"nbOfTimeCrashedPDFP", nbOfTimeCrashedPDFP},
	         {"nbOfTimeAgreedWithAll", nbOfTimeAgreedWithAll},
	         {"lastTestWasAllGood", lastTestWasAllGood},
	         {"lastTestWasOnlySlow", lastTestWasOnlySlow},
	         {"driversThatCannotParseThis", driversThatCannotParseThis},
	         {"checksum", checksum}}};
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
			if ( not stats.checksum.empty() )
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
			stats.checksum.push_back( hi < 10 ? hi + '0' : 'A' + hi - 10 );
			stats.checksum.push_back( lo < 10 ? lo + '0' : 'A' + lo - 10 );
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
		execl( g_process.c_str(),
		       g_process.c_str(),
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
			auto lines = su::split_view<char>( output, '\n' );
			if ( lines.size() >= 2 )
			{
				if ( su::starts_with( lines[lines.size()-2], "time: " ) )
				{
					result.t = std::stoull( std::string(lines[lines.size()-2].substr( 6 )) );
				}
				if ( su::starts_with( lines[lines.size()-1], "mem: " ) )
				{
					result.m = std::stoull( std::string(lines[lines.size()-1].substr( 5 )) );
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
	auto allFiles = getAll( input );

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
		if ( skip > 0 and fileStats.lastTestWasAllGood )
			continue;
		if ( skip > 1 and fileStats.lastTestWasOnlySlow )
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
		if ( fileStats.nbOfTimeTested == 0 )
		{
			// new file
			ostr << R"(<td><font color="#FF4F0A">)" << pdfFile.stem()
			     << "</font></td>";
		}
		else if ( fileStats.nbOfTimeTested > 100 and
		          fileStats.nbOfTimeAgreedWithAll > 100 )
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
			if ( fileStats.canBeParseWith( it ) )
				testResults[it] = test_driver( it, pdfFile );
			else
			{
				TestResult tr;
				tr.unableToParse = true;
				testResults[it] = tr;
			}
		}
		++fileStats.nbOfTimeTested;

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
		fileStats.lastTestWasOnlySlow = false;
		int nbOfGoodResult = 0;
		for ( auto &it : drivers )
		{
			auto &ds = driverStats[it];
			auto &r = testResults[it];

			if ( r.unableToParse )
			{
				ostr << cell( "----" );
				++nbOfGoodResult;
			}
			else if ( r.crash )
			{
				ostr << cell( "Crash", true, "#F00" );
				++ds.nbOfCrash;
				if ( it == "pdfp" )
					++fileStats.nbOfTimeCrashedPDFP;
			}
			else if ( r.exit != 0 )
			{
				ostr << cell( "Invalid", true );
				++ds.nbOfFail;
			}
			else if ( not r.diff.empty() )
			{
				ostr << cell( "OK", true, "#0C0", r.diff );
			}
			else if ( referenceTime > r.t or referenceMem > r.m )
			{
				ostr << cell(
				    "OK", false, "#00C", diffPerf( referenceTime, r.t, referenceMem, r.m ) );
				fileStats.lastTestWasOnlySlow = true;
			}
			else
			{
				++nbOfGoodResult;
				ostr << cell( "OK", false, "#0C0", diffPerf( referenceTime, r.t, referenceMem, r.m ) );
			}
		}

		if ( nbOfGoodResult == drivers.size() )
		{
			++fileStats.nbOfTimeAgreedWithAll;
			fileStats.lastTestWasAllGood = true;
		}
		else
			fileStats.lastTestWasAllGood = false;

		// update filestats
		saveFileStats( pdfFile, fileStats );

		ostr << "</tr>" << std::endl;
	}
	ostr << "</tbody></table>\n<br/><br/>\n";

	// driver stats

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
				drivers.push_back( n );
		}
		g_process = argv[0];
		run_tests( su::filepath( input ), drivers, skip );
	}
	else
		return usage();
	return 0;
}
