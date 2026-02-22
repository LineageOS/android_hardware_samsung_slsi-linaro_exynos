#!/bin/bash
echo "=================================================================================================="
echo "Usage: ./build_npuc_android.sh [Build Option] [Log Filter]"
echo "=================================================================================================="
echo ""
echo "Build Option"
echo "   clean | c                                   Clean build and delete NPU_Compiler directory"
echo "   rel* | r                                    Release libs and include files after build"
echo "   build* | b                                  Build with debugging"
echo "   download* | d                               Clone npuc git (download npuc git)"
echo ""
echo "Example"
echo "   $./build_npuc_android.sh r c d       for release and clean directory"
echo "   $./build_npuc_android.sh r d         for release"
echo "   $./build_npuc_android.sh d           for download and clone npuc git"
echo "   $./build_npuc_android.sh b           for build and debug with RelWithDebInfo mode"
echo "=================================================================================================="
echo ""

if [ $# -eq 0 ]
then
    echo "No Options, set to default build"
else
    echo "Parsing Options..."
fi

CLEAN_JOB="false"
RELEASE_JOB="false"
DEBUG_MODE="false"
CLONE_GIT="false"

while true;
do
    case "$1" in
      clean* | c)
        echo "   Clean build and delete NPU_Compiler directory"
        CLEAN_JOB="true"
        ;;
      rel* | r)
        echo "   Add RELEASE_JOB"
        RELEASE_JOB="true"
        ;;
      build_debug* | b)
        echo "   Build with debug"
        DEBUG_MODE="true"
        ;;
      download* | d)
        echo "   Clone npuc git (download npuc git)"
        CLONE_GIT="true"
        ;;
      *)
        echo "   Unknown argument: $1"
        break
        ;;
    esac
    shift
done


echo ""
echo "Start building npuc"
echo ""

build_ret=0

NPUC_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

echo "NPUC_ROOT = ${NPUC_ROOT}"

if [ "$CLONE_GIT" == "true" ];then
    if [ -d "${NPUC_ROOT}/NPU_Compiler/" ]; then
        echo "Delete NPU_Compiler"
        rm -rf  ${NPUC_ROOT}/NPU_Compiler/
    fi
    echo "Clone NPUC git"
    git clone https://npuc.id:tkatjdakstp1!@github.sec.samsung.net/EDEN/NPU_Compiler.git
fi

if [ ! -d "${NPUC_ROOT}/NPU_Compiler/ci" ]; then
    echo ""
    echo "Fail to clone npuc git"
    echo ""
    exit
else
    echo ""
    echo "Pull npuc git"
    echo ""
    pushd ${NPUC_ROOT}/NPU_Compiler
    git pull
    popd
fi

TARGETS="arm aarch64"

BUILD_LOG=${NPUC_ROOT}/build.log

rm -f ${BUILD_LOG}

for TARGET in ${TARGETS}
do
    echo "Build ${TARGET} for ondevice compiler"
    pushd ${NPUC_ROOT}/NPU_Compiler/ci
    if [ -d "../Product/" ]; then
        echo "Delete Product directory"
        rm -rf ../Product/
    fi

    if [ "$DEBUG_MODE" == "false" ];then
        echo "Build with Release"
        ./run_all.sh -lp run --cmakeargs "-DDRU_ROUNDING=1 -DCMAKE_BUILD_TYPE=Release -DLIB_TYPE=SHARED" --compiler_type "ondevice" --docker_build 0 --target_arch ${TARGET}  2>&1 | tee -a ${BUILD_LOG} 
    else
        echo "Build with RelWithDebInfo"
        ./run_all.sh -lp run --cmakeargs "-DDRU_ROUNDING=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLIB_TYPE=SHARED" --compiler_type "ondevice" --docker_build 0 --target_arch ${TARGET} 2>&1 | tee -a ${BUILD_LOG}
    fi

    while read line
    do
        if [[ "$line" =~ "warning:" || "$line" =~ "note:" ]]
        then
            read
        elif [[ "$line" =~ "error:" || "$line" =~ "errors generated" || "$line" =~ "error generated" ]]
        then
            if [ $build_ret == 0 ]
            then
                echo -e "\n\n\n*** Build error list is below ***\n"
            fi
            echo "$line"
            build_ret=1
        fi
    done < ${BUILD_LOG}

    popd

    pushd ${NPUC_ROOT}/NPU_Compiler
    if [ $build_ret == 0 ]
    then
        echo -e "\n\n*** BUILD SUCCEEDED ***\n"
    else
        echo -e "\n\n*** BUILD FAILED ***\n"
    fi

    echo "Remove build log ${BUILD_LOG}"
    rm -f ${BUILD_LOG}

    if [ "$RELEASE_JOB" == "true" ];then
        echo ""
        echo "Copy libs and include files"
        echo ""

        OUT_DIR=${NPUC_ROOT}/../
        echo "Copying deliverables => $OUT_DIR"
        if [ ! -d "./Product/out/lib/" ]; then
            echo ""
            echo "Product/out/lib/ does not exist."
            echo ""
            exit
        fi

        if [ "$TARGET" == "aarch64" ];then
            rm -f $OUT_DIR/lib64/*.so
            cp -f ./Product/out/lib/* $OUT_DIR/lib64/
        else
            rm -f $OUT_DIR/lib32/*.so
            cp -f ./Product/out/lib/* $OUT_DIR/lib32/
        fi
        rm -rf $OUT_DIR/include/npuc/*
        cp -r ./Product/out/include/npuc/*  $OUT_DIR/include/npuc/
        echo ""
        echo "Release Completed"
        echo ""
    fi
    popd
done

if [ "$CLEAN_JOB" == "true" ];then
    if [ -d "${NPUC_ROOT}/NPU_Compiler/" ]; then
        echo "Delete NPU_Compiler directory ${NPUC_ROOT}/NPU_Compiler/"
        rm -rf  ${NPUC_ROOT}/NPU_Compiler/
    fi
fi
