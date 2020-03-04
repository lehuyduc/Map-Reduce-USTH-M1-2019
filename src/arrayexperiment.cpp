#include "arrayexperiment.h"

ArrayExperiment::ArrayExperiment(vector<double> newInputs, Distribution newDistribution) : inputs(newInputs), distribution(newDistribution) {

}

//----------------------------------------------    LINEAR ALGORITHM
iFloat ArrayExperiment::linearTest(const vector<double> &inputs, Op op) {
    if (inputs.size()==0)
        throw ArrayExperimentException("Linear test error: input vector is empty");

    qDebug() << "before linear test\n";
    double res = 0;
    for (unsigned i=0; i<inputs.size(); i++) res = numOperate(res, inputs[i], op);
    qDebug() << "after linear test\n";
    return res;
}


//----------------------------------------------    SPLIT/MERGE ALGORITHM
double ArrayExperiment::splitMerge(const vector<double> &inputs, Op op, int l, int r) {
    if (l>r)
        throw ArrayExperimentException("SplitMerge error: l can't be > r for valid inputs");

    if (l==r) return inputs[l];
    int mid = (l+r)/2;
    return numOperate(splitMerge(inputs, op, l, mid), splitMerge(inputs, op, mid+1, r), op);
}

iFloat ArrayExperiment::splitMergeTest(const vector<double> &inputs, Op op) {
    if (inputs.size()==0)
        throw ArrayExperimentException("Split merge test error: input vector is empty");

    double res = splitMerge(inputs, op, 0, inputs.size() - 1);

    return res;
}

iFloat ArrayExperiment::sortTest(vector<double> inputs, Op op) {
    if (inputs.size()==0)
        throw ArrayExperimentException("Sort test error: input vector is empty");

    std::sort(inputs.begin(), inputs.end());

    double res = 0;
    for (unsigned i=0; i<inputs.size(); i++) res = numOperate(res, inputs[i], op);

    return res;
}

// first, sort the array. Then repeat N-1 times:
// 1. get 2 smallest number, sum them and remove them from array.
// 2. Push the sum back into the array
// 3. repeat
iFloat ArrayExperiment::sortAppendTest(vector<double> inputs, Op op) {
    if (inputs.size()==0)
        throw ArrayExperimentException("Sort test error: input vector is empty");

    // we have to use pointer because input size can be very large, which might cause stack overflow
    std::priority_queue<double, vector<double>, std::greater<double> > *minHeap = new std::priority_queue<double, vector<double>, std::greater<double> >;
    for (unsigned i=0; i<inputs.size(); i++) minHeap->push(inputs[i]);

    for (unsigned i=1; i<inputs.size(); i++) {
        double a = minHeap->top();
        minHeap->pop();
        double b = minHeap->top();
        minHeap->pop();
        minHeap->push(numOperate(a, b, op));
    }

    double res = minHeap->top();
    delete minHeap;

    return res;
}


//---------------------------------------------     EXPERIMENTING
// for an experiment, we random shuffle the input nTest times.
// For each shuffle, we calculate the result of each algorithm

iFloat ArrayExperiment::groundTruth(const vector<double> &inputs, Op op) {
    iFloat res = 0;
    for (unsigned i=0; i<inputs.size(); i++) res = numOperate(res, iFloat(inputs[i]), op);
    return res;
}

vector<Result> ArrayExperiment::experiment(Op op, unsigned int nTest, vector<Algo> testAlgos, bool shuffle=true) {
    ArrayGenerator arrGen(distribution);
    vector<Result> res;
    res.clear();

    res.push_back(Result(shuffle, GROUND_TRUTH)); // if we use shuffle it means a ground truth exist, so output 1
    res.push_back(Result(groundTruth(inputs, op), GROUND_TRUTH));
    qDebug() << "After ground truth\n";

    bool linear = 0, splitMerge = 0, sortLinear = 0, sortAppend = 0;
    for (unsigned i=0; i<testAlgos.size(); i++) {
        if (testAlgos[i]==LINEAR) linear = true;
        if (testAlgos[i]==SPLIT_MERGE) splitMerge = true;
        if (testAlgos[i]==SORT) sortLinear = true;
        if (testAlgos[i]==SORT_APPEND) sortAppend = true;
    }

    qDebug() << "after boolean testAlgos\n";

    for (unsigned t=1; t<=nTest; t++) {
        if (linear) res.push_back(Result(linearTest(inputs, op), LINEAR));
        if (splitMerge) res.push_back(Result(splitMergeTest(inputs, op), SPLIT_MERGE));
        if (sortLinear) res.push_back(Result(sortTest(inputs, op), SORT));
        if (sortAppend) res.push_back(Result(sortAppendTest(inputs, op), SORT_APPEND));

        qDebug() << "after running algorithms\n";
        if (shuffle) std::random_shuffle(inputs.begin(), inputs.end());
        else {
            arrGen.generateArray(inputs.size(), inputs);
        }
        emit signalUpdateProgress(t*100/nTest);
    }

    return res;
}

//--------------------------------------------  SIGNALS AND SLOTS FOR THREAD FUNCTIONS
void ArrayExperiment::slotRunArrayExperiment(Op op, unsigned nTest, vector<Algo> testAlgos, bool shuffle) {
    vector<Result> res = experiment(op, nTest, testAlgos, shuffle);
    emit signalExperimentFinish(res);
}
