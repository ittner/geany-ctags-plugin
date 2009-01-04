print "BEGIN\n";
while ( <STDIN>)
{
	chomp ;
	$i++;
	print "[$i] (($_))\n" ;
}
print "END\n";
