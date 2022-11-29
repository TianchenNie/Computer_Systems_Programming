for i in {0..40}
do
    sh list_test.sh
    sh reduction_test.sh
    echo "Total test $i finished."
done