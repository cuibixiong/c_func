
[1] SHELL COLOR OUTPUT

RED="0;31m"
GREEN="0;32m"
YELLOW="0;33m"
BLUE="0;34m"

function printf()
{
    local level=$1
    local msg=$2

    if [[ ${level} = "EMERGE" ]];then
        echo  -e "\033[${RED} EMERGE: '${msg}'\033[00m"
    elif [[ ${level} = "WARNING" ]];then
        echo  -e "\033[${YELLOW} WARNING: '${msg}'\033[00m"
    elif [[ ${level} = "MESSAGE" ]];then
        echo  -e "\033[${GREEN} MESSAGE: '${msg}'\033[00m"
    fi
}

printf EMERGE  "hello world"
printf WARNING "hello world"
printf MESSAGE "hello world"

[2] ADD DEBUG INFO TO FUNCTION FOR C CODE

find -name '*.c' -exec sed -i '/)$/N;s#)\n{#)\n{\n\tprintk(\"my_debug:
%s %d %s\\n\", __FILE__, __LINE__, __FUNCTION__);#' {} \;

[3] SORT NUM


232332 [19851016] 0913
232332 [19900713] 0125
340403 [19820811] 1312
350783 [19860519] 3316
370683 [19840308] 4132
500234 [19850121] 6453
622421 [19830905] 2213
 
awk '{a[substr($0,7,8)]=$0}END{for(i=1;i<=asorti(a,b);i++)print a[b[i]]}' 

[4] getopt

declare -A options

TEMP=`getopt -o vh: --long dir:,ignore-dir:,ignore-test:,mail:,testsuite:,tool:,target:,help,verbose: -n 'example.bash' -- "$@"`

if [ $? != 0 ];then echo "Terminating..." >&2; exit 1;fi

eval set -- "$TEMP"

while true; do
    case "$1" in
        -v | --verbose)
            options["verbose"]=1;
            shift;
            ;;
        -h | --help)
            help
            shift;
            ;;
        --mail)
            options["mail"]=$2
            shift 2;
            ;;
        --ignore-dirs)
            options["ignoredirs"]=$2
            shift 2;
            ;;
        --ignore-tests)
            options["ignoretests"]=$2
            shift 2;
            ;;
        --testsuite)
            options["testsuite"]=$2
            shift 2;
            ;;
        --tool)
            options["tool"]=$2
            shift 2;
            ;;
        --output)
            options["outdir"]=$2
            shift 2;
            ;;
        --target)
            options["target"]=$2
            shift 2;
            ;;
        --) shift ; break;;
        *) echo "Error, No such opitons support"; exit 1;
    esac
done

