
filesdir=$1
searchstr=$2

if [ $# -ne 2 ]; then
    echo "Arguments were not given well"
    exit 1
fi

if ! [ -d $filesdir ]; then
    echo "$filesdir is not a valid path"
    exit 1
fi

files_num=$(grep -rl ${searchstr} ${filesdir} | wc -l)
match_num=$(grep -or ${searchstr} ${filesdir} | wc -w)

echo "The number of files are ${files_num} and the number of matching lines are ${match_num}"
