kit: PS1.c
        gcc PS1.c -o kit

samples: kit
        @echo "Sample 1 - input with singular input and output file"
        echo "Sample Input" > input1.txt
        ./kit.exe -o output1.txt input.txt
        @echo "Sample 2 - multiple inputs and stdin to stdout"
        echo "Sample Input2" > input2.txt
        echo "Sample Input3" > input3.txt
        ./kit input2.txt - input3.txt
        @echo "Sample 3 - input from stdin output from stdout"
        ./kit
        @echo "Error Sample 1 - no input file"
        -./kit non-existent-file
        @echo "Error Sample 2 - multiple output files specified"
        -./kit -o out1 -o out2
        @echo "Error Sample 3 - no permission to read"
        touch noReadingPerm
        chmod -r noReadingPerm
        -./kit noReadingPerm
        @echo "Error Sample 4 - no permission to write"
        touch noWritingPerm
        chmod -w noWritingPerm
        -./kit -o noWritingPerm


clean:
        rm -f kit *.txt