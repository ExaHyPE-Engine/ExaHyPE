#!/usr/bin/env bash
mainFile=solution-ADERDGEuler.peano-patch-file
dataset=ADERDGEulerQ

# filter
rm -rf output-filtered/*
echo "filter"
../../../src/convert/convert apply-filter ${mainFile} ${dataset} output-filtered extract-fine-grid ${dataset}-fine-grid
# convert
rm -rf output-vtu/*
echo "convert"
../../../src/convert/convert convert-file output-filtered/${mainFile} ${dataset}-fine-grid output-vtu vtu
# rename
echo "rename"
cd output-vtu
for file_in in *.vtu; do
   snapshot=$(echo "${file_in}" | grep -e "[0-9]\+" -o)
   file_out=${file_in/-$snapshot/}
   echo "mv ${file_in} ${file_out/.vtu/-${snapshot}.vtu}"
   mv ${file_in} ${file_out/.vtu/-${snapshot}.vtu}
done
cd ..

rm -rf output-filtered/*
