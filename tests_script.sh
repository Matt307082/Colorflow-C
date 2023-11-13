#!/bin/bash


#gcc "./colorflow.c" -o "./colorflow" -lpng -ljpeg /usr/include/libbmp/libbmp.c

IMAGES_DIRECTORY="./pictures"
PASSED_TESTS=0
EXECUTED_TESTS=0

for IMAGE_FILE in "$IMAGES_DIRECTORY"/*; do

    if [ -f "$IMAGE_FILE" ]; then

        IDENTIFIER=$(basename "$IMAGE_FILE" | cut -d'.' -f1)

        RESULT=$(./colorflow -f${IMAGE_FILE} -n 10)


        
        if [ "${RESULT:0:6}" = "$IDENTIFIER" ] && [ $? -eq 0 ]; then
            echo "Test $IMAGE_FILE ok"
            let "PASSED_TESTS+=1"
        else
            echo "Test $IMAGE_FILE failed"
            echo "-----------------------------------------------"
            echo "Expected: $IDENTIFIER"
            echo "Got: ${RESULT:0:6}"
            echo "-----------------------------------------------"
        fi
        let "EXECUTED_TESTS+=1"
    fi
done
echo "Tests: $PASSED_TESTS passed, $EXECUTED_TESTS total"