#include <iostream>
#include <string>
#include <stdlib.h>
#include <chrono>
#include <assert.h>
using namespace std;
using namespace std::chrono;


#define CONFIG_B    50
#define CONFIG_N    1024
#define CONFIG_K    20

float RandomFloat(float min, float max){
    float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
    return min + scale * ( max - min );      /* [min, max] */
}

void InitTensor(float *tensor, int dim0, int dim1, int dim2, int mode){
    unsigned long indxD;
    
    for(int d0=0; d0<dim0; d0++){
        for(int d1=0; d1<dim1; d1++){
            for(int d2=0; d2<dim2; d2++){
                indxD = d0 * dim1*dim2 +
                        d1 * dim2 +
                        d2;
                if(mode==0){
                    tensor[indxD] = RandomFloat(-2.0f,2.0f);
                }else if(mode==1){
                    tensor[indxD] = d2;
                }else if(mode==2){
                    tensor[indxD] = 10*(d1*d0) + d2;
                }
            }
        }
    }
    
}

/*
inputTn:    INPUT - Distance tensor of rank three (dim0xdim1xdim2), row-major
indicesTn:  OUTPUT- Indices of top 'k' elements for each dim2 slice of inputTn (dim0xdim1xK)
outputTn:   OUTPUT- Fully sorted version of inputTn (dim0xdim1xdim2)
*/
void BatchSelectionSortTopK(
    const float* inputTn,
    int* indicesTn,
    float* outputTn,
    int dim0,
    int dim1,
    int dim2,
    int kValue){

    int i, j, max_idx;  
    
    assert(kValue<dim2);

    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    // 1. Copy inputTn into outputTn, so sorting algorithm could be
    //    run on outputTn without editing inputTn.
    for(unsigned long i = 0; i<dim0*dim1*dim2; i++){
        outputTn[i] = inputTn[i];
    }

    // 2. Initializing indicesTn for each of k-element slices of it.
    for(int batch=0; batch<dim0*dim1; batch++){
        for(int i=0; i<kValue; i++){
            indicesTn[batch*kValue + i] = i;
        }
    }

    // 3. Running descending selection sort only for first k elements of
    //    each of dim2 slices of outputTn(which is a clone of inputTn).
    for(int batch=0; batch<dim0*dim1; batch++){

        // Run selection sort on current slice of dim2.
        for (i = 0; i < kValue; i++)  
        {  
            // Find the maximum element in unsorted array  
            max_idx = i;  
            for (j = i+1; j < dim2; j++){
                if (outputTn[batch*dim2 + j] > outputTn[batch*dim2 + max_idx])
                    max_idx = j;  
            }
            
            // Swap the found maximum element with the first element  
            if(max_idx != i){
                float tmp = outputTn[batch*dim2 + max_idx];
                outputTn[batch*dim2 + max_idx] = outputTn[batch*dim2 + i];
                outputTn[batch*dim2 + i] = tmp;
                //------------------------------------------------------------
                int tmpi = indicesTn[batch*kValue + max_idx];
                indicesTn[batch*kValue + max_idx] = indicesTn[batch*kValue + i];
                indicesTn[batch*kValue + i] = tmpi;
            }
        } 

    }

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>( t2 - t1 ).count();
    cout << "Execution Time (" << __func__ << ") (microseconds): "<< duration << endl;
}

void PrintTensor(
    const float *tensor,
    int dim0,
    int dim1,
    int dim2,
    string strname,
    int limitDim0=1,
    int limitDim1=10){

    assert(limitDim0<dim0);
    assert(limitDim1<dim1);

    unsigned long indxD;

    cout<<endl<<endl<<"DUMP: "<< strname<<endl;
    for(int d0=0; d0<limitDim0; d0++){
        for(int d1=0; d1<limitDim1; d1++){
            for(int d2=0; d2<dim2; d2++){
                indxD = d0*dim1*dim2 + d1*dim2 + d2;
                cout<< strname << "[d0,d1,d2: "<< d0 << ", " << d1 << ", " << d2 << "] =\t" << tensor[indxD] << endl;
            }
        }
    }

    cout<<endl;
}

int main(){
    float *distanceTn = new float[CONFIG_B * CONFIG_N * CONFIG_N];
    float *sortedTn = new float[CONFIG_B * CONFIG_N * CONFIG_N];
    int *indicesTn = new int[CONFIG_B * CONFIG_N * CONFIG_K];

    // 1. Init batch of distance matrices of NxN. (BxNxN)
    InitTensor(distanceTn, CONFIG_B, CONFIG_N, CONFIG_N, 0);

    // 2. Run batch-topk op on distance tensor.
    BatchSelectionSortTopK(distanceTn, indicesTn, sortedTn, CONFIG_B, CONFIG_N, CONFIG_N, CONFIG_K);

    // 3. Dumping some of the sortedTn dim2 slices for the user.
    PrintTensor(sortedTn, CONFIG_B, CONFIG_N, CONFIG_N, "sortedTn");

}