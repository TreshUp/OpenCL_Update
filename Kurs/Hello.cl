__kernel void square_fwd1(__global float* Value, __global int* Col, __global int* RowIndex, int j,__global float* koef) 
 { 
	 koef[(((1+j)*j)/2)+j]=sqrt(fabs(Value[RowIndex[j]]));
 }
__kernel void square_fwd2(__global float* Value, __global int* Col, __global int* RowIndex, int i,__global float* koef)
 { 
	float s;
	int j = get_global_id(0); 
	 if ((j)>i)
	 { 
		if (j==(Col[RowIndex[i]+j - Col[RowIndex[i]]]))
		{
			s = Value[RowIndex[i]+j - Col[RowIndex[i]]];
			for(int k = 0; k < i; k++)
			{
				s-=koef[(((1+j)*j)/2)+k]*koef[(((1+i)*i)/2)+k];
			} 
 
		}
		else s=0;
			s/=koef[(((1+i)*i)/2)+i];
		
			koef[(((1+j)*j)/2)+i]=s;
			Value[RowIndex[j]] -= s*s;
     }
 }
__kernel void square_y1(__global float* x,__global float* Sv, int i,__global float* koef) 
 { 

	 x[i]=Sv[i]/koef[(((1+i)*i)/2)+i];
 } 
__kernel void square_y2(__global float* x,__global float* Sv, int i,__global float* koef) 
 { 
     local float y; 
     y = x[i]; 
 
 
     int k = get_global_id(0); 
 

     if(k > i)
	 { 
		 Sv[k]-=y * koef[(((1+k)*k)/2)+i];
     } 
 } 
 
 
__kernel void square_x1(__global float *x, int i,__global float* koef) 
 { 
	x[i]/=koef[(((1+i)*i)/2)+i];
 } 
 
 
__kernel void square_x2(__global float *x, int i,__global float* koef) 
 { 
     local float y; 
     y = x[i]; 
 
 
     int k = get_global_id(0); 
	if (k<i)
	{
		x[k]-=y*koef[(((1+i)*i)/2)+k];
	 }
 }