use strict;

while (<STDIN>)
{
    if (/<source>([^<]*)/)
    {
	my $source = $1;
	print "<pre><code>";
	open(Source, "<$source") || die "Can't open $source";
	while (<Source>) { print; }
	close(Source);
	print "</pre></code>\n";
    }
    elsif (/<run>([^<]*)/)
    {
	my $prog = $1;
	print "<pre><code>";
	open(Prog, "./$prog |") || die "Can't open $prog";
	while (<Prog>)
	{
	    s/&/&amp;/g;
	    s/</&lt;/g;
	    print;
	}
	close(Prog);
	print "</pre></code>\n";
    }
    else
    {
	while (/<code class="ref">([^<]*)<.code>/)
	{
	    my $target = $1;
	    s@$&@<a href="#$target"><code>$target</code></a>@;
	}
	print;
    }
}
