__kernel void square_fwd1(__global double* Value, __global int* Col, __global int* RowIndex, int j,__global float* koef) 
 { 
	 //Value[RowIndex[j]]=sqrt(fabs(Value[RowIndex[j]]));
	 koef[(((1+j)*j)/2)+j]=sqrt(fabs(Value[RowIndex[j]]));
 }
__kernel void square_fwd2(__global double* Value, __global int* Col, __global int* RowIndex, int i,__global float* koef, __global float* tmp)//int j) 
 { 
	float s;
	int j = get_global_id(0); 
	 if ((j)>i)
	 { 
		 //s = Value[RowIndex[i]+j - Col[RowIndex[i]]];
		if (j==(Col[RowIndex[i]+j - Col[RowIndex[i]]]))
		{
			s = Value[RowIndex[i]+j - Col[RowIndex[i]]];
			for(int k = 0; k < i; k++)
			{
				//s -= Value[RowIndex[k]+j - Col[RowIndex[k]]]*Value[RowIndex[k]+i - Col[RowIndex[k]]];
				s-=koef[(((1+j)*j)/2)+k]*koef[(((1+i)*i)/2)+k];
				//s[0]-=koef[(((1+k)*k)/2)+j]*koef[(((1+k)*k)/2)+i];
				///s-=koef[(((1+k)*k)/2)+j]*koef[(((1+k)*k)/2)+i];
			} 
 
		}
		else s=0;
			//s/=Value[RowIndex[i]];
			s/=koef[(((1+i)*i)/2)+i];
		
			//Value[RowIndex[i]+j - Col[RowIndex[i]]] = s;
			koef[(((1+j)*j)/2)+i]=s;
			///koef[(((1+i)*i)/2)+j]=s;
			Value[RowIndex[j]] -= s*s;
     }
 }
__kernel void square_y1(__global double* Value, __global int* Col, __global int* RowIndex, __global float* x,__global float* Sv, int i,__global float* koef) 
 { 
     //x[i] = U[n-1+i*n] / U[i+i*n]; 
	 ////x[i]=Sv[i]/Value[RowIndex[i]];
	 x[i]=Sv[i]/koef[(((1+i)*i)/2)+i];
	 //x[i]=111;
 } 
__kernel void square_y2(__global double* Value, __global int* Col, __global int* RowIndex, __global float* x,__global float* Sv, int i,__global float* koef) 
 { 
     local float y; 
     y = x[i]; 
 
 
     int k = get_global_id(0); 
 

     if(k > i)
	 { 
         //U[n-1+k*n] -= U[k+i*n] * y; 
		 //Sv[k]-=Sv[i]*y;
		 //Sv[k]-=koef[(((1+k)*k)/2)+i]*y;
		 Sv[k]-=y * koef[(((1+k)*k)/2)+i];
     } 
 } 
 
 
__kernel void square_x1(__global double* Value, __global int* Col, __global int* RowIndex, __global float *x, int i,__global float* koef) 
 { 
     //x[i] /= U[i+i*n];
	//x[i]/= Value[RowIndex[i]];
	x[i]/=koef[(((1+i)*i)/2)+i];
 } 
 
 
__kernel void square_x2(__global double* Value, __global int* Col, __global int* RowIndex, __global float *x, int i,__global float* koef) 
 { 
     local float y; 
     y = x[i]; 
 
 
     int k = get_global_id(0); 
	if (k<i)
	{
		//x[k] -= y * U[i+k*n]; 
		//x[k]-=y*Value[RowIndex[k]+i - Col[RowIndex[k]]];
		x[k]-=y*koef[(((1+i)*i)/2)+k];
	 }
 }