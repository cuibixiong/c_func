sub do_command
{
	my $cmd = shift;
	my $description = @_ ? shift : "command";
	my $die_on_fail = @_ ? shift : undef;
	$debug and print "% $cmd\n";
	system ($cmd);
	if ($? == -1) 
	{
        $debug and printf ("error: %s failed to execute: $!\n", $description);
		$die_on_fail and $? and exit(1);
		return $?;
    }
    elsif ($? & 127) 
	{
        $debug and printf("error: %s child died with signal %d, %s coredump\n", 
						  $description, 
						  ($? & 127),  
						  ($? & 128) ? 'with' : 'without');
		$die_on_fail and $? and exit(1);
		return $?;
    }
    else 
	{
		my $exit = $? >> 8;
		if ($exit)
		{
			$debug and printf("error: %s child exited with value %d\n", $description, $exit);
			$die_on_fail and exit(1);
		}
		return $exit;
    }
}

