#! /bin/sh
# korn shell script to help create the medium res .rsp file
# Author: G. Powell 4/8/97

rm goo.txt
echo '# MEDRTRES.RSP for RESUTIL, ' `date` > goo.txt
echo '# Expects to live in a subdirectory off the main BIRTHRT directory.' >> goo.txt

for fline in `cat foo.txt`
do echo $fline
   echo '+ ..\\things\\'$fline'\\'$fline's*.flc' >> goo.txt
   echo '+ ..\\things\\'$fline'\\'$fline'w*.flc' >> goo.txt
   echo '+ ..\\things\\'$fline'\\'$fline'x*.flc' >> goo.txt
   echo '+ ..\\things\\'$fline'\\'$fline'c*.flc' >> goo.txt
   echo '+ ..\\things\\'$fline'\\'$fline'l*.flc' >> goo.txt
   echo '+ ..\\things\\'$fline'\\'$fline'g*.flc' >> goo.txt
   echo '+ ..\\things\\'$fline'\\'$fline'b*.flc' >> goo.txt
   echo '+ ..\\things\\'$fline'\\'$fline'm*.flc' >> goo.txt
   echo '+ ..\\things\\'$fline'\\'$fline'z*.flc' >> goo.txt
done

echo "Done."
