list=`ls expected/*.ssa`
for i in $list
do
    echo $i
    diff "${i}" "/home/yida/cs352/uscc/tests/${i}"
done