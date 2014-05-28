#!/usr/local/bin/perl5
#
# Create SNMP manpages
#
# Author: Ryan Troll <ryan@andrew.cmu.edu>
#
# $Id: snmp-man.pl,v 1.1.1.1 2001/11/15 01:29:36 panther Exp $
#
######################################################################

## All manpages are listed here
##
# mibii.o
@MIBII = ("snmpInASNParseErrs", "snmpInASNParseErrs_Add",
	  "snmpInBadVersions", "snmpInBadVersions_Add");
$Functions{"snmp_mibii"} = \@MIBII;
$Pages{"snmp_mibii"} = "MIBII glue";

# snmp_error.o
@ERROR = ("snmp_errstring");
$Functions{"snmp_error"} = \@ERROR;
$Pages{"snmp_error"} = "PDU Error Functions";

# coexistance.o
@COEXIST = ("snmp_coexist_V2toV1", "snmp_coexist_V1toV2");
$Functions{"snmp_coexistance"} = \@COEXIST;
$Pages{"snmp_coexistance"} = "V1/V2 Coexistance Functions";

# snmp_extra.o
@EXTRA = ("uptime_string", "myaddress",
	  "mib_TxtToOid", "mib_OidToTxt");
$Functions{"snmp_extra"} = \@EXTRA;
$Pages{"snmp_extra"} = "Miscellaneous Functions";

# snmp_msg.o
@Msg = ("snmp_msg_Encode", "snmp_msg_Decode");
$Functions{"snmp_msg"} = \@Msg;
$Pages{"snmp_msg"} = "Message Encoding / Decoding Functions";

# snmp_pdu.o
@PDU = ("snmp_pdu_create", "snmp_pdu_clone",
	"snmp_pdu_fix",    "snmp_free_pdu",
	"snmp_pdu_encode", "snmp_pdu_decode",
	"snmp_pdu_type");
$Functions{"snmp_pdu"} = \@PDU;
$Pages{"snmp_pdu"} = "PDU Functions";

# snmp_vars.o
@Var = ("snmp_vars_new", "snmp_var_clone",
	"snmp_var_free", "snmp_var_EncodeVarBind",
	"snmp_var_DecodeVarBind");
$Functions{"snmp_vars"} = \@Var;
$Pages{"snmp_vars"} = "Variable Bindings Functions";

# snmp_dump.o
@Dump = ("snmp_dump_packet", "snmp_dump");
$Functions{"snmp_packet_dump"} = \@Dump;
$Pages{"snmp_packet_dump"} = "Network Packet Dumping Functions";

# snmp_client.o
@Client = ("snmp_synch_input", "snmp_synch_response", "snmp_synch_setup");
$Functions{"snmp_client"} = \@Client;
$Pages{"snmp_client"} = "Client communication routines";

# snmp_api.o
@API = ("snmp_open", "snmp_build", "snmp_parse", "snmp_send", "snmp_read",
	"snmp_select_info", "snmp_timeout", "snmp_close", "snmp_api_stats");
$Functions{"snmp_api"} = \@API;
$Pages{"snmp_api"} = "Session Management API";

# new session

@SESS = ( "snmp_sess_init", "snmp_sess_open", "snmp_sess_session",
	  "snmp_sess_send", "snmp_sess_async_send", "snmp_sess_read",
	  "snmp_sess_select_info", "snmp_sess_timeout", "snmp_sess_close",
	  "snmp_sess_error");
$Functions{"snmp_sess_api"} = \@SESS;
$Pages{"snmp_sess_api"} = "SNMP Single Session API";

# snmp_api_error.o
@ApiErr = ("snmp_api_error", "snmp_api_errno");
$Functions{"snmp_api_errors"} = \@ApiErr;
$Pages{"snmp_api_errors"} = "API Error Functions";

# mini-client.o
@Mini = ("init_mini_snmp_client", "close_mini_snmp_client", 
	 "snmp_mini_response_len", "snmp_mini_open", "snmp_mini_close",
	 "snmp_mini_set_int", "snmp_mini_set_str",
	 "snmp_mini_get", "snmp_mini_getnext");
$Functions{"snmp_mini_client"} = \@Mini;
$Pages{"snmp_mini_client"} = "Mini-client";

# mib.o
@MIB = ("init_mib", "read_objid", "print_objid", "sprint_objid",
	"print_variable", "sprint_variable",
	"print_value", "sprint_value",
	"print_variable_list", "print_variable_list_value", "print_type",
	"print_oid_nums");
$Functions{"snmp_mib"} = \@MIB;
$Pages{"snmp_mib"} = "MIB Dependant Functions";

# parse XXXXX

# version.o
@Ver = ("snmp_Version");
$Functions{"snmp_version_info"} = \@Ver;
$Pages{"snmp_version_info"} = "Version Information";

########################################################################

if ($ARGV[0] eq "-create") {

  &CreatePages();

} elsif ($ARGV[0] eq "-install") {
  $INSTALL=$ARGV[1];
  $MAN=$ARGV[2];

  $MAN3DIR="$MAN/man3";
  $MAN5DIR="$MAN/man5";

  &InstallPages();

} else {
  die "Must specify -create or -install!";
}


########################################################################

# Create all manpages in the current directory
sub CreatePages {

  close(STDOUT);

  # -----------------------------------------------------------------------

  foreach $Page (keys %Functions) {

    ## List of functions for this page
    ##
    $FuncList = $Functions{$Page};

    print STDERR "Generating manpage for '$Page'\n";
    open(STDOUT, "> ${Page}.3") ||
      die "Unable to write to ${Page}.3: $!";

    ## Write the manpage
    &Header($Page);
    &Cat("${Page}.man");
    &RFCs();
    &URLs();
    &SeeAlso();
    close(STDOUT);

    ## Now write the tiny pointer files
    foreach $Func (@$FuncList) {
      &Pointer($Func, $Page);
    }
  }

  # -----------------------------------------------------------------------

  ## Create OID description
  open(STDOUT, "> snmp_oid.5") || die "Unable to write to snmp_oid.5: $!";
  &Header("snmp_oid");
  &Cat("snmp_oid.man");
  &RFCs();
  &URLs();
  &SeeAlso();
  close(STDOUT);

  # -----------------------------------------------------------------------

  ## Now make the main page
  open(STDOUT, "> snmp.3") || die "Unable to write to snmp.3: $!";
  &Header("snmp");
  &Cat("snmp.man");

  foreach $Page (sort keys %Pages) {
    print STDOUT ".TP 5\n";
    print STDOUT ".SB $Page\n";
    print STDOUT "$Pages{$Page}\n";
  }

  &Environment();
  &Files();
  &RFCs();
  &URLs();
  &SeeAlso();
  close(STDOUT);

  # -----------------------------------------------------------------------

}

###########################################################################

sub InstallPages {

  print STDOUT "Installing into $MAN with '$INSTALL'\n";

  foreach $Page (keys %Functions) {
    &Install("${Page}.3", $MAN3DIR);

    ## List of functions for this page
    ##
    $FuncList = $Functions{$Page};

    ## Now write the tiny pointer files
    foreach $Func (@$FuncList) {
      &Install("${Func}.3", $MAN3DIR);
    }
  }

  &Install("snmp.3",     $MAN3DIR);
  &Install("snmp_oid.5", $MAN5DIR);
}

###########################################################################

## Cat the body of a file to STDOUT
##
sub Cat {
  my($F) = @_;

  open(A, $F) || die "Unable to read $F: $!";
  while(<A>) {
    print STDOUT $_;
  }
  close(A);
}

## The header for our function manpages
##
sub Header {
  my($Page) = @_;
  my($Time) = scalar localtime;

  $Page =~ y/a-z/A-Z/;
  print STDOUT ".TH $Page 3 \"$Time\"\n";
  print STDOUT ".UC 4\n";
}

## Environment variables
##
sub Environment {

  print STDOUT ".SH \"ENVIRONMENT\"\n";
  print STDOUT "MIBFILE:  Location of the SNMP MIB.\n";

}

## Files used by the CMU SNMP Library
##
sub Files {
  print STDOUT ".SH \"FILES\"\n";
  print STDOUT ".nf\n";
  print STDOUT "mib.txt                   First MIB tried if env. var is not set\n";
  print STDOUT "/etc/mib.txt              Second MIB tried if env. var is not set\n";
}

## Related RFCs
##
sub RFCs {
  print STDOUT ".SH \"RFCS\"\n";
  print STDOUT "Related RFCs: 1065, 1066, 1067\n";
  print STDOUT ".br\n";
  print STDOUT "Related SNMPv2 RFCs: 1901, 1902, 1902, 1904, 1905, 1906, 1907, 1908, 1909\n";
}

## Related URLs
##
sub URLs {
  print STDOUT ".SH \"RELATED URLS\"\n";
  print STDOUT "CMU Networking Group: http://www.net.cmu.edu/\n";
  print STDOUT ".br\n";
  print STDOUT "CMU SNMP Home Page: http://www.net.cmu.edu/projects/snmp\n";
}

## The infamous see also section.
##

sub SeeAlso {
  my($l, $n, $s);
  my($Page);
  print STDOUT ".SH \"SEE ALSO\"\n";
  
  $l = 0;
  $s = 0;
  foreach $Page (sort keys %Pages) {
    print STDOUT ".BR ${Page} (3),\n";
  }
  print STDOUT ".BR snmp_oid (5)\n";
  print STDOUT ".BR snmp (3)\n";
}


sub Pointer {
  my($Function, $Page) = @_;

  open(FOO, "> $Function.3") || die "Unable to open $Function.3: $!";
  print FOO ".so man3/$Page.3\n";
  close(FOO);
}


sub Install {
  my($File, $Dir) = @_;

  system("${INSTALL} $File $Dir");
}
