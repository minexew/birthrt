#! /bin/sh
# Strip out multiplayer lines.

# Make sure there is the sed control file.
if	test ! -r 'rand.sed'
then	print 'Missing rand.sed file.'
	exit
fi

filelist=
arg=$1
	while true
	do case $arg in
		-help)	echo 'remove multiplayer text from log file.'
			echo '$0 filenames' > output file.
			;;
		*)	if	test -r "$arg"		# test for readable file.
			then filelist="$filelist$arg "
			else	echo "File $arg doesn't exist."
				exit
			fi
			;;
		esac
		shift;
		arg=$1
		if 	test "$arg" = ''
		then	break;
		fi
	done

for files in $filelist
do
	sed -f rand.sed "$files"
done
