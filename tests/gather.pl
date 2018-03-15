#!/opt/local/bin/perl
#

use Digest::MD5;
use File::Basename;

print "---> searching for all using spotlight\n";
@all_files = `find /Users/sandy/projets/fileformats/PDFs -iname '*.pdf'`;

my %allCheckSums = {};

`rm -rf ~/pdf_links`;
`mkdir ~/pdf_links`;

print "---> gathering all files\n";
foreach ( @all_files )
{
	chomp;
	my $current_file = $_;
	
	if ( $current_file =~ /^\/private\// )	# skip /private
	{
		next;
	}
	my $ck = filechecksum( $current_file );
	if (  defined $allCheckSums{$ck} )
	{
		next;
	}
	$allCheckSums{$ck} = $current_file;
	my $fname = basename( $current_file );
	if ( -e "$ENV{HOME}/pdf_links/$fname" )
	{
		my $index = 1;
		my $newName = "$fname" . "_$index.pdf";
		while ( -e "$ENV{HOME}/pdf_links/$newName" )
		{
			$index++;
			$newName = "$fname" . "_$index.pdf";
		}
		`cd ~/pdf_links ; mv "$current_file" "$newName"`;
	}
	else
	{
		`cd ~/pdf_links ; mv "$current_file" .`;
	}
}

sub filechecksum
{
	my $file = shift;
	open(FILE, $file) or die "Can't open '$file': $!";
	binmode(FILE);
	my $md5 = Digest::MD5->new;
	while (<FILE>)
	{
		$md5->add($_);
	}
	close(FILE);
	return $md5->hexdigest;
}
