#!/bin/bash

array=()
occurs=()

dirName="outs/"

# array of TLD names
for args do
    array+=("."${args})
    occurs+=("0")
done


# take all files
for file in $(find $dirName -type f)
do
    exec < "$file"

    # read each line
    while read line
    do
        for ((i=0; i < ${#array[@]}; i++))
        do
            if [[ "${line}" == *"${array[i]}"* ]]; then
                for ((j=0; j<${#line}; j++))
                do
                    if [ "${line:j:1}" = " " ];
                    then
                        toBeAdded="${line:j:$((${#line}-$j))}"
                        occurs[i]=$((occurs[i]+toBeAdded))
                        break
                    fi
                done

            fi

        done
    done
done

# print TLD occurances
for ((i=0; i < ${#array[@]}; i++))
do
    echo ${array[i]} ${occurs[i]}

done
