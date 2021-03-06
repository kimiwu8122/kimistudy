#!/bin/sh
#---Filename----------------------------------------------------------
CountCfg=Reset.num
LOGOUT=SystemError.log
MCEOUT=mce.out
PCIOUT=pci.out
SATACOMOUT=satacompare.out
PCInow=pcinow.txt
SATAnow=satanow.txt
AEROUT=aer.out
LANEOUT=lane.out
SATAOUT=sata.out
USBOUT=usb.out
RESETOUT=reset.out
DefPCIFile=DefaultPCI.txt       # Default PCI file
DefSATAFile=DefaultSATA.txt     # Default SATA file
#---------------------------------------------------------------------

#---Test Program------------------------------------------------------
RegTool=pciscan32
#---------------------------------------------------------------------

stringdate="$(date +%Y%m%d)_$(date +%H%M%S)"

short_sync_fun() {
	sync
	sync
	sleep 1
}

long_sync_fun() {
	sync
	sync
	sync
	sleep 4
}

# Check whether file exist
checkfile_fun() {
	test -f $2 && FileExist=TRUE || FileExist=FALSE
	if [ "$FileExist" = "TRUE" ] && [ "$1" = "rename" ]; then
		tempname=`echo "$2" | cut -d "." -f 1`
		tempname="${tempname}_${stringdate}"
		tempname1=`echo "$2" | cut -d "." -f 2`
		tempname="${tempname}.${tempname1}" 
		mv $2 $tempname
	elif [ "$FileExist" = "FALSE" ] && [ "$1" = "exit" ]; then
		echo "ERROR : $2 is not exist."
		exit
	fi
}

mce_fun() {
	mcetmp=mce.tmp
	mcelog > $mcetmp
	leng=`cat $mcetmp|wc -l 2>&1`
	echo > $MCEOUT
	if [ $leng -eq 0 ]; then
		echo "** MCE Check: PASS" >> $MCEOUT
	else
		echo "** MCE Check: FAIL" >> $MCEOUT
		echo "================================================" >> $MCEOUT
		cat $mcetmp >> $MCEOUT
		echo "================================================" >> $MCEOUT
	fi
	rm -f $mcetmp
}

# PCI Compare Test
pci_compare() {
	num=1
	leng=`cat $1|wc -l 2>&1`
	while [ $num -le $leng ]
	do
		PCIDevice=`cat $1|head -n $num|tail -n 1`
		printf '   -%-63s:%s\n' "$PCIDevice" "$2" >> $PCITMP3
		num=$(($num + 1))
	done
}

# PCI Test
pci_fun() {
	checkfile_fun "exit" $1
	checkfile_fun "exit" $2
	
	echo "" > $PCIOUT
	PCIFile1=$1
	PCIFile2=$2
	PCITMP0=temppci0
	PCITMP1=temppci1
	PCITMP2=temppci2
	PCITMP3=temppci3
	
	if [ `diff "$PCIFile1" "$PCIFile2"|wc -l` -gt 0 ]; then
		Judgment="FAIL"
		diff "$PCIFile1" "$PCIFile2"|grep -v "\-\-\-"|grep -v "+++"|grep -v "@@" > $PCITMP0
		cat $PCITMP0|grep "^\-"|cut -c 2- > $PCITMP1
		cat $PCITMP0|grep "^+"|cut -c 2- > $PCITMP2

		rm -f $PCITMP3
		pci_compare $PCITMP1 "Exist->Null"
		pci_compare $PCITMP2 "Null->Exist"
	else
		Judgment="PASS"
	fi
	
	if [ "$Judgment" = "FAIL" ]; then
		echo "** PCI SCAN Compare Test: FAIL" >> $PCIOUT
		cat $PCITMP3 >> $PCIOUT
	elif [ "$Judgment" = "PASS" ]; then
		echo "** PCI SCAN Compare Test: ALL PASS" >> $PCIOUT
	fi
	rm -f $PCITMP0 $PCITMP1 $PCITMP2 $PCITMP3
}

# SATA Compare Test
sata_compare_fun() {
	checkfile_fun "exit" $1
	checkfile_fun "exit" $2
	
	echo "" > $SATACOMOUT
	SATAFile1=$1
	SATAFile2=$2
	
	SATATMP0=tempsata0
	SATATMP1=tempsata1
	SATATMP2=tempsata2
	
	if [ `diff "$SATAFile1" "$SATAFile2"|wc -l` -gt 0 ]; then
		Judgment="FAIL"
		diff "$SATAFile1" "$SATAFile2"|grep -v "\-\-\-"|grep -v "+++"|grep -v "@@" > $SATATMP0
		cat $SATATMP0|grep "^\-"|cut -c 2- > $SATATMP1
		cat $SATATMP0|grep "^+"|cut -c 2- > $SATATMP2
		
	else
		Judgment="PASS"
	fi
	
	if [ "$Judgment" = "FAIL" ]; then
		echo "** SATA Compare Test: FAIL" >> $SATACOMOUT
		cat $SATATMP1 >> $SATACOMOUT
		cat $SATATMP2 >> $SATACOMOUT
	elif [ "$Judgment" = "PASS" ]; then
		echo "** SATA Compare Test: ALL PASS" >> $SATACOMOUT
	fi
	rm -f $SATATMP0
}

aer_fun() {
	aertmp=aer.tmp
	./$RegTool -error aer > $aertmp
	error=`cat $aertmp|grep ": Error"`
	echo > $AEROUT
	if [ -z "$error" ]; then
		echo "** PCI AER Check: PASS" >> $AEROUT
	else
		echo "** PCI AER Check: FAIL" >> $AEROUT
		cat $aertmp >> $AEROUT
	fi
	rm -f $aertmp
}

pci_lane_fun() {
	lanetmp=lane.tmp
	./$RegTool -error lane > $lanetmp
	error=`cat $lanetmp|grep ": Error"`
	echo > $LANEOUT
	if [ -z "$error" ]; then
		echo "** PCI Lane Error Check: PASS" >> $LANEOUT
	else
		echo "** PCI Lane Error Check: FAIL" >> $LANEOUT
		cat $lanetmp >> $LANEOUT
	fi
	rm -f $lanetmp
}

sata_fun() {
	satatmp=sata.tmp
	satatmp1=sata1.tmp
	./$RegTool -error sata > $satatmp
	cat $satatmp|grep "FAIL" > $satatmp1
	echo > $SATAOUT
	if [ `cat $satatmp1|wc -l` -ne 0 ]; then
		echo "** SATA Check: FAIL" >> $SATAOUT
	else
		echo "** SATA Check: PASS" >> $SATAOUT
		cat $satatmp >> $SATAOUT
	fi
	rm -f $satatmp $satatmp1
}

usb_fun() {
	usbtmp=usb.tmp
	usbtmp1=usb1.tmp
	./$RegTool -error usb > $usbtmp
	cat $usbtmp|grep "FAIL" > $usbtmp1
	echo > $USBOUT
	if [ `cat $usbtmp1|wc -l` -ne 0 ]; then
		echo "** USB Check: FAIL" >> $USBOUT
                cat $usbtmp >> $USBOUT
	else
		echo "** USB Check: PASS" >> $USBOUT
	fi
	rm -f $usbtmp $usbtmp1
}

count_fun() {
	echo "CPUResetCount $CPUResetCount" > $CountCfg
	echo "WarmResetCount $WarmResetCount" >> $CountCfg
	echo "ColdResetCount $ColdResetCount" >> $CountCfg
	echo "Counter $(( $Count +1 ))" >> $CountCfg
	long_sync_fun
}

reset_fun() {
	echo > $RESETOUT

	if [ $ColdResetCount -lt 100 ]; then
		echo "** Reset Type: Cold Reset"  >> $RESETOUT
		cat $RESETOUT
		cat $RESETOUT                  >> $LOGOUT
		rm -f $RESETOUT

		echo "CPUResetCount $CPUResetCount" > $CountCfg
		echo "WarmResetCount $WarmResetCount" >> $CountCfg
		echo "ColdResetCount $(( $ColdResetCount +1 ))" >> $CountCfg
		echo "Counter $Count" >> $CountCfg

		long_sync_fun
		ioport w 0xcf9 0xe
	elif [ $WarmResetCount -lt 100 ]; then
		echo "** Reset Type: Warm Reset"  >> $RESETOUT
		cat $RESETOUT
		cat $RESETOUT                  >> $LOGOUT
		rm -f $RESETOUT

		echo "CPUResetCount $CPUResetCount" > $CountCfg
		echo "WarmResetCount $(( $WarmResetCount +1 ))" >> $CountCfg
		echo "ColdResetCount $ColdResetCount" >> $CountCfg
		echo "Counter $Count" >> $CountCfg

		long_sync_fun
		ioport w 0xcf9 0x6
	elif [ $CPUResetCount -lt 100 ]; then
		echo "** Reset Type: CPU Reset"  >> $RESETOUT
		cat $RESETOUT
		cat $RESETOUT                  >> $LOGOUT
		rm -f $RESETOUT

		echo "CPUResetCount $(( $CPUResetCount +1 ))" > $CountCfg
		echo "WarmResetCount $WarmResetCount" >> $CountCfg
		echo "ColdResetCount $ColdResetCount" >> $CountCfg
		echo "Counter $Count" >> $CountCfg

		long_sync_fun
		ioport w 0xcf9 0x4
	fi
     
}

reset1_fun() {
	echo > $RESETOUT

	if [ $ColdResetCount -gt 0 ]; then
		echo "** Reset Type: Cold Reset"  >> $RESETOUT
		cat $RESETOUT
		cat $RESETOUT                  >> $LOGOUT
		rm -f $RESETOUT

		echo "CPUResetCount $CPUResetCount" > $CountCfg
		echo "WarmResetCount $WarmResetCount" >> $CountCfg
		echo "ColdResetCount $(( $ColdResetCount -1 ))" >> $CountCfg
		echo "Counter $Count" >> $CountCfg

		long_sync_fun
		ioport w 0xcf9 0xe
	elif [ $WarmResetCount -gt 0 ]; then
		echo "** Reset Type: Warm Reset"  >> $RESETOUT
		cat $RESETOUT
		cat $RESETOUT                  >> $LOGOUT
		rm -f $RESETOUT

		echo "CPUResetCount $CPUResetCount" > $CountCfg
		echo "WarmResetCount $(( $WarmResetCount -1 ))" >> $CountCfg
		echo "ColdResetCount $ColdResetCount" >> $CountCfg
		echo "Counter $Count" >> $CountCfg

		long_sync_fun
		ioport w 0xcf9 0x6
	elif [ $CPUResetCount -gt 0 ]; then
		echo "** Reset Type: CPU Reset"  >> $RESETOUT
		cat $RESETOUT
		cat $RESETOUT                  >> $LOGOUT
		rm -f $RESETOUT

		echo "CPUResetCount $(( $CPUResetCount -1 ))" > $CountCfg
		echo "WarmResetCount $WarmResetCount" >> $CountCfg
		echo "ColdResetCount $ColdResetCount" >> $CountCfg
		echo "Counter $Count" >> $CountCfg

		long_sync_fun
		ioport w 0xcf9 0x4
	fi
     
}

#---Main function----------------------------------------------------- 
if [ -z $1 ]; then
	echo "================================================================================"
	echo "System Error Check tool V1.72 , Release Date: 2017/03/07"
	echo "Copyright (C) 2013 by Sean Chou . 2017 Maintain by Kimi Wu" 
	echo "================================================================================"
	
	echo "Usage:"
	echo "./sec.sh -mce               # Check MCE error"
	echo "         -pci               # PCI compare test"
#	echo "         -satacompare       # SATA compare test"
	echo "         -aer               # Check PCI AER error"
	echo "         -lane              # Check PCI Lane error"
	echo "         -sata              # Check SATA error"
	echo "         -reset             # Warm reset and cold reset"
	echo "         -count             # counter + 1"
	echo "         -first             # Times set to 1"
	echo "         -all               # include -mce -aer -lane -sata"
	echo "         -init              # Save default PCI List file"
	echo
	echo "examp:"
	echo "./sec.sh -init              # Save default PCI List file"
	echo "./sec.sh -all -reset -first # Do first test and then reset"
	echo "./sec.sh -all -count -first # Do first test for Power On/Off test"
        echo "./sec.sh -all -reset -opcount # Do reset by option count"
	exit
fi

init="no"; mce="no"; first="no" pci="no"; aer="no"; lane="no"; sata="no"; all="no"; debug="no"; reset="no"; usb="no"; count="no"; satacompare=="no"; opcount="no"

for parameter in $1 $2 $3 $4 $5 $6 $7 $8 $9
do
	if [ "$parameter" = "-mce"  ] || [ "$parameter" = "-all" ]; then mce="yes"  ; fi
	if [ "$parameter" = "-pci"  ] || [ "$parameter" = "-all" ]; then pci="yes"  ; fi
#	if [ "$parameter" = "-satacompare"  ] || [ "$parameter" = "-all" ]; then satacompare="yes"  ; fi
	if [ "$parameter" = "-aer"  ] || [ "$parameter" = "-all" ]; then aer="yes"  ; fi
	if [ "$parameter" = "-lane" ] || [ "$parameter" = "-all" ]; then lane="yes"  ; fi
	if [ "$parameter" = "-sata" ] || [ "$parameter" = "-all" ]; then sata="yes" ; fi
#	if [ "$parameter" = "-usb"  ] || [ "$parameter" = "-all" ]; then usb="yes"  ; fi
	if [ "$parameter" = "-init" ]; then init="yes"; fi
	if [ "$parameter" = "-first" ]; then first="yes" ; fi
	if [ "$parameter" = "-count" ]; then count="yes" ; fi
	if [ "$parameter" = "-debug"  ]; then debug="yes" ; fi
	if [ "$parameter" = "-reset" ]; then reset="yes"; fi
        if [ "$parameter" = "-opcount" ]; then opcount="yes"; fi
done 

# Check Tool
checkfile_fun "exit" $RegTool

if [ "$init" = "yes" ]; then
	# Create Default PCI file
	./$RegTool -lspci > $DefPCIFile
#	./$RegTool -sata > $DefSATAFile
	short_sync_fun
	exit
fi

if [ "$first" = "yes" ] || [ ! -f $CountCfg ]; then 
	# Check log file
	checkfile_fun "rename" $LOGOUT
	
	if [ "$reset" = "yes" ]; then 
		echo "CPUResetCount 0" > $CountCfg
		echo "WarmResetCount 0" >> $CountCfg
		echo "ColdResetCount 1" >> $CountCfg
		echo "Counter 0" >> $CountCfg
	else
		echo "CPUResetCount 0" > $CountCfg
		echo "WarmResetCount 0" >> $CountCfg
		echo "ColdResetCount 0" >> $CountCfg
		echo "Counter 1" >> $CountCfg
	fi
fi

HEARD="<==========================   System Error Test   ==========================>"
echo $HEARD
echo $HEARD	                          >> $LOGOUT

CPUResetCount=`cat $CountCfg|grep "CPUResetCount"|cut -d " " -f 2`
WarmResetCount=`cat $CountCfg|grep "WarmResetCount"|cut -d " " -f 2`
ColdResetCount=`cat $CountCfg|grep "ColdResetCount"|cut -d " " -f 2`
Count=`cat $CountCfg|grep "Counter"|cut -d " " -f 2`

echo "CPU Reset Count : $CPUResetCount" >> $LOGOUT
echo "Warm Reset Count : $WarmResetCount" >> $LOGOUT
echo "Cold Reset Count : $ColdResetCount" >> $LOGOUT
echo "Count : $Count" >> $LOGOUT
echo "CPU Reset Count : $CPUResetCount"
echo "Warm Reset Count : $WarmResetCount"
echo "Cold Reset Count : $ColdResetCount"
echo "Count : $Count"

# MCE error check Start
if [ "$mce" = "yes" ]; then
	mce_fun
	cat $MCEOUT
	cat $MCEOUT                  >> $LOGOUT
	rm -f $MCEOUT
fi

# PCI Compare Start
if [ "$pci" = "yes" ]; then
	./$RegTool -lspci > $PCInow
	pci_fun $DefPCIFile $PCInow
	cat $PCIOUT
	cat $PCIOUT                  >> $LOGOUT
	rm -f $PCIOUT $PCInow
fi

# SATA Compare Start
if [ "$satacompare" = "yes" ]; then
	./$RegTool -sata > $SATAnow
	sata_compare_fun $DefSATAFile $SATAnow
	cat $SATACOMOUT
	cat $SATACOMOUT                  >> $LOGOUT
	rm -f $SATACOMOUT $SATAnow
fi

# PCI AER check Start
if [ "$aer" = "yes" ]; then
	aer_fun
	cat $AEROUT
	cat $AEROUT                  >> $LOGOUT
	rm -f $AEROUT
fi

# PCI Lane error check Start
if [ "$lane" = "yes" ]; then
	pci_lane_fun
	cat $LANEOUT
	cat $LANEOUT                 >> $LOGOUT
	rm -f $LANEOUT
fi

# SATA error check Start
if [ "$sata" = "yes" ]; then
	sata_fun
	cat $SATAOUT
	cat $SATAOUT                  >> $LOGOUT
	rm -f $SATAOUT
fi

# USB check Start
if [ "$usb" = "yes" ]; then
	usb_fun
	cat $USBOUT
	cat $USBOUT                  >> $LOGOUT
	rm -f $USBOUT
fi

short_sync_fun

# counter + 1, no reset
if [ "$count" = "yes" ]; then
	count_fun
fi

# Reset Start
if [ "$reset" = "yes" ] && [ "$opcount" = "no" ]; then
	reset_fun
fi
# Reset Start
if [ "$reset" = "yes" ] && [ "$opcount" = "yes" ]; then
	reset1_fun
fi
