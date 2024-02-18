writefile=$1
writestr=$2

if [ $# -ne 2 ]; then
    echo "Arguments were not given well"
    exit 1
fi


(mkdir -p ${writefile%/*} && echo "${writestr}" > ${writefile}) || echo "File could not be created"