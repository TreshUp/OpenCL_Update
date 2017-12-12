__kernel void square_fwd1(__global double* Value, __global int* Col, __global int* RowIndex, int j,__global float* koef) 
 { 
	 //Value[RowIndex[j]]=sqrt(fabs(Value[RowIndex[j]]));
	 koef[((1+j)*j)/2]=sqrt(fabs(Value[RowIndex[j]]));
 }
__kernel void square_fwd2(__global double* Value, __global int* Col, __global int* RowIndex, int i,__global float* koef)//int j) 
 { 
	 //int i = get_global_id(0); 
	 int j = get_global_id(0); 
	//TO DO
     //if(i > j)
	 if (j>i)
	 { 
		 float s = Value[RowIndex[i]+j - Col[RowIndex[i]]];
		 //if (s!=0)
		 //{
		 //float s = Value[RowIndex[j]+i - Col[RowIndex[j]]];
 
			for(int k = 0; k < i; k++)
			{
			//for(int k = 0; k < j; k++) 
				//s -= Value[RowIndex[k]+i - Col[RowIndex[k]]]*Value[RowIndex[k]+j - Col[RowIndex[k]]];
				//s -= Value[RowIndex[k]+j - Col[RowIndex[k]]]*Value[RowIndex[k]+i - Col[RowIndex[k]]];
				s-=koef[(((1+j)*j)/2)+k]*koef[(((1+i)*i)/2)+k];
			} 
 
 
			//s/=Value[RowIndex[i]];
			///s/=koef[((1+i)*i)/2];
			//s/=Value[RowIndex[j]];
		
			//Value[RowIndex[i]+j - Col[RowIndex[i]]] = s;
			koef[(((1+j)*j)/2)+i]=s;
			//Value[RowIndex[j]+i - Col[RowIndex[j]]] = s;
		//}
 
		Value[RowIndex[j]] -= s*s;
		//Value[RowIndex[i]] -= s*s;
     } 
 }
__kernel void square_y1(__global double* Value, __global int* Col, __global int* RowIndex, __global float *x,__global float *Sv, int i) 
 { 
     //x[i] = U[n-1+i*n] / U[i+i*n]; 
	 x[i]=Sv[i]/Value[RowIndex[i]];
 } 
 
 
__kernel void square_y2(__global double* Value, __global int* Col, __global int* RowIndex, __global float *x,__global float *Sv, int i) 
 { 
     local float y; 
     y = x[i]; 
 
 
     int k = get_global_id(0); 
 

     if(k > i)
	 { 
         //U[n-1+k*n] -= U[k+i*n] * y; 
		 Sv[k]-=Sv[i]*y;
     } 
 } 
 
 
__kernel void square_x1(__global double* Value, __global int* Col, __global int* RowIndex, __global float *x, int i) 
 { 
     //x[i] /= U[i+i*n];
	x[i]/= Value[RowIndex[i]];
 } 
 
 
__kernel void square_x2(__global double* Value, __global int* Col, __global int* RowIndex, __global float *x, int i) 
 { 
     local float y; 
     y = x[i]; 
 
 
     int k = get_global_id(0); 
 
     //x[k] -= y * U[i+k*n]; 
	 x[k]-=y*Value[RowIndex[k]+i - Col[RowIndex[k]]];
 }