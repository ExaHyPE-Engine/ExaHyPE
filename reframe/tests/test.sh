if [ $# -eq 0 ]
  then
    echo "No arguments supplied"
fi
export REVISION="${1}"
echo $REVISION
