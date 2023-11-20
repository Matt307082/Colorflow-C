FILES="pictures/*"
for f in $FILES
do
  echo "Processing $f file..."
  # take action on each file. $f store current file name
  ./colorflow -f $f > $f.result
done