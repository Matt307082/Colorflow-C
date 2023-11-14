#!/bin/bash


#gcc "./colorflow.c" -o "./colorflow" -lpng -ljpeg /usr/include/libbmp/libbmp.c

IMAGES_DIRECTORY="./pictures"
PASSED_TESTS=0
EXECUTED_TESTS=0

for IMAGE_FILE in $IMAGES_DIRECTORY/*.bmp $IMAGES_DIRECTORY/*.jpeg $IMAGES_DIRECTORY/*.png; do

    if [ -f "$IMAGE_FILE" ]; then

        RESULT=$(./colorflow -f${IMAGE_FILE})

        EXPECTED="$(cat $IMAGE_FILE.result)"
        
        if [ $? -eq 0 ] && [ "$RESULT" = "$EXPECTED" ]; then
            echo "Test $IMAGE_FILE ok"
            let "PASSED_TESTS+=1"
        else
            echo "Test $IMAGE_FILE failed"
            echo "-----------------------------------------------"
            echo "Expected: $IMAGE_FILE.result"
            echo "Got: $RESULT"
            echo "-----------------------------------------------"
        fi
        let "EXECUTED_TESTS+=1"
    fi
done
echo "Tests: $PASSED_TESTS passed, $EXECUTED_TESTS total"
