mkdir -p testfolder
mkdir -p testfolder/folder1
mkdir -p testfolder/folder2
mkdir -p testfolder/folder3
mkdir -p testfolder/folder1/folder4
mkdir -p testfolder/folder1/folder4/folder5

touch testfolder/folder1/file1.txt
touch testfolder/folder1/folder4/file2.txt
touch testfolder/folder1/folder4/folder5/file3.txt
touch testfolder/file4.txt
touch testfolder/file5.txt
touch testfolder/folder2/file6.txt
touch testfolder/folder3/file7.txt
touch testfolder/folder3/file8.txt
touch testfolder/folder3/file9.txt

files=$(find testfolder -type f)
filenames=$(basename -a $files)
IFS=$'\n'
readarray -t files <<< "$files"
readarray -t filenames <<< "$filenames"
echo ${files[@]}
echo ${filenames[@]}
filelen=${#files[@]}

for (( i=0; i<filelen; i++ )); do
    lines=$(echo ${filenames[i]} | sed 's/[^0-9]*//g')
    echo "This is ${filenames[i]}! I have $(($lines)) lines. This is line 1." > ${files[i]}
    for (( j=2; j<=$((lines)); j++ )); do
        echo "This is line ${j}." >> ${files[i]}
    done
done
