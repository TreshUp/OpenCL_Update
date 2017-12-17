#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <Windows.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <math.h>

#include <conio.h>
#include <stdio.h>

#define N 10000
#define local_NZ 5
#define MAX_SOURCE_SIZE (0x100000)

using namespace std;
struct crsMatrix
{
	// ������ ������� �������� (������ NZ)
	int* Col;
	// ������ �������� ����� (������ N + 1)
	int* RowIndex;
	// ������ �������� (������ NZ)
	float* Value;
};

void InitializeMatrix(crsMatrix &mtx)
{

	mtx.Value= (float*)calloc(local_NZ*N, sizeof(float));
	mtx.Col = (int*)calloc(local_NZ*N, sizeof(int));
	mtx.RowIndex = (int*)calloc(N + 1, sizeof(int));
}
void FreeMemory(crsMatrix &mtx,float* X, float* Sv, float* koef, char* str, cl_platform_id * platforms, cl_device_id *device_list, cl_mem Value_clmem, cl_mem Col_clmem, cl_mem Row_Index_clmem)
{

	free(mtx.Value);
	free(mtx.Col);
	free(mtx.RowIndex);
	free(X);
	free(Sv);
	free(koef);
	free(str);
	free(platforms);
	free(device_list);
	clReleaseMemObject(Value_clmem);
	clReleaseMemObject(Col_clmem);
	clReleaseMemObject(Row_Index_clmem);
}
inline void FreeKernel(cl_kernel kernel)
{
	cl_int clStatus = clReleaseKernel(kernel);
}
void SetKernel(cl_kernel kernel, cl_mem Value_clmem, cl_mem Col_clmem, cl_mem Row_Index_clmem, cl_mem koef_clmem)
{
	cl_int clStatus;
	clStatus = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&Value_clmem);
	clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&Col_clmem);
	clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&Row_Index_clmem);
	clStatus = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&koef_clmem);
}
void SetKernel(cl_kernel kernel, cl_mem X_clmem, cl_mem Sv_clmem, cl_mem koef_clmem)
{
	cl_int clStatus;
	clStatus = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&X_clmem);
	clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&Sv_clmem);
	clStatus = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&koef_clmem);
}
void SetKernel(cl_kernel kernel, cl_mem X_clmem, cl_mem koef_clmem)
{
	cl_int clStatus;
	clStatus = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&X_clmem);
	clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&koef_clmem);
}
void LenGenerate(crsMatrix &mtx, float* Sv)
{
	//srand(time(NULL));
	int i, j = 0;
	mtx.RowIndex[0] = 0;
	for (i = 0; i < N - 1; i++)
	{
		int  Num_Non_zero = 1 + local_NZ / 2;
		Sv[i]= rand() % 100 + 1;
		mtx.RowIndex[i + 1] = mtx.RowIndex[i] + Num_Non_zero;
		int	tmp = i;
		while (Num_Non_zero != 0)
		{
			if (tmp==i) mtx.Value[j] = rand() % 5 + 11;
			else mtx.Value[j] = rand() % 5 + 1;
			mtx.Col[j] = tmp;
			Num_Non_zero--;
			tmp += 1;
			j++;
		}
	}
	Sv[N-1] = rand() % 100 + 1;
	mtx.RowIndex[N] = mtx.RowIndex[N - 1] + local_NZ / 2;
	mtx.Value[j] = rand() % 5 + 11;
	mtx.Col[j] = N - 1;
}
void MapleCheck(crsMatrix &mtx,float* Sv)
{
	float** Check = (float**)calloc(N, sizeof(float*));
	int k = 0;
	for (k = 0; k < N; k++)
	{
		Check[k]= (float*)calloc(N, sizeof(float));
	}
	int i = 0;
	while (i < N)
	{
		int position = mtx.RowIndex[i];
		int j = i;
		while (position <= (mtx.RowIndex[i + 1] - 1))
		{
			Check[i][j] = mtx.Value[position];
			//cout << i << " " << j << endl;
			if ((j > i) && (j<N)) Check[j][i] = mtx.Value[position];
			position++;
			j++;
		}
		i++;
	}
	ofstream fout;
	fout.open("Sv.txt", ios::out);
	for (k = 0; k < N; k++)
	{
		fout << Sv[k]<<endl;
	}
	fout.close();
	fout.setf(ios::fixed);
	fout.precision(0);
	fout.open("Maple.txt", ios::out);
	for (k = 0; k < N; k++)
	{
		for (int j = 0; j < N; j++)
		{
			fout << Check[k][j]<<" ";
		}
		fout << endl;
	}
	fout.close();
	for (k = 0; k < N-1; k++)
	{
		free(Check[k]);
	}
	free(Check);
}
inline cl_kernel InitKernel(cl_program program, const char* str, cl_int clStatus)
{
	return clCreateKernel(program, str, &clStatus);
}
double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		cout << "QueryPerformanceFrequency failed!\n";

	PCFreq = double(li.QuadPart);// / 1000.0;

	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;
}
double GetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}
int main()
{
	crsMatrix mtx;
	//��������� ������
	InitializeMatrix(mtx);
	float *Sv = (float*)calloc(N, sizeof(float));
	LenGenerate(mtx,Sv);

#pragma region OpenCL
#pragma region READ
	//��� ��� ��� ������ ������� �� ����� Hello.cl
	FILE *fp;
	const char fileName[] = "../Kurs/Hello.cl";
	size_t source_size;
	char *source_str;
	int i;

	try {
		fp = fopen(fileName, "r");
		if (!fp) {
			fprintf(stderr, "Failed to load kernel.\n");
			exit(1);
		}
		source_str = (char *)malloc(MAX_SOURCE_SIZE);
		source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);//��������� ������ �������� MAX_SOURCE_SIZE, ������ ��������  1 // ���������� ����� ������� ��������� ���������
		fclose(fp);
	}
	catch (int a) {
		printf("%f", a);
	}
#pragma endregion
	float *X = (float*)calloc(N, sizeof(float));
	float *koef = (float*)calloc(((1+N)*N)/2.0, sizeof(float*));

	// Get platform and device information
	cl_platform_id * platforms = NULL; //������ ��������� ��������
	cl_uint num_platforms; //���-�� ��������

	//Set up the Platform
	cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms); //������ �������� - ����� �������� �� "add"; �� ������ � num_platforms �������� ����� ��������
	platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms); //������������ ��������� ������
	clStatus = clGetPlatformIDs(num_platforms, platforms, NULL); //��������� ������ ��������� ��������; �� ������ � ������� �����. ��� ���������

	//Get the devices list and choose the device you want to run on
	cl_device_id *device_list = NULL; //������ ��������� ���������
	cl_uint num_devices; //���-�� ���������
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices); //������ �������� - ����� ���������; ������ - ���; ������ - ����� ��������� �� ����������; �� ������ � num_devices �������� ����� ���������
	device_list = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);//������������ ��������� ������
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, num_devices, device_list, NULL); //��������� ������ ��������� ���������; �� ������ � ������� �����. ��� ����������

	// Create one OpenCL context for each device in the platform
	cl_context context;
	context = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &clStatus); //�������� ���������
	// Create a command queue
	cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus); // �������� ������� �� ������ ��� ����������;properties - ������ ������� ������� ������


	// Create memory buffers on the device for each vector
    //��������� ������ ��� ������� �� ��������
	cl_mem Value_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, local_NZ*N * sizeof(float), NULL, &clStatus);
	cl_mem Col_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, local_NZ*N * sizeof(int), NULL, &clStatus);
	cl_mem Row_Index_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, (N + 1) * sizeof(int), NULL, &clStatus);
	cl_mem X_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, N * sizeof(float), NULL, &clStatus);
	cl_mem Sv_clmem= clCreateBuffer(context, CL_MEM_READ_WRITE, N * sizeof(float), NULL, &clStatus);
	cl_mem koef_clmem= clCreateBuffer(context, CL_MEM_READ_WRITE, ((1 + N)*N) / 2 * sizeof(float), NULL, &clStatus);

	// Copy the Buffer A and B to the device
	//����������� �������� � � � �� ������ ����� � �����; CL_TRUE - ������ � �� ����������� � ����� ������ ������� clEnqueueWriteBuffer

	clStatus = clEnqueueWriteBuffer(command_queue, Value_clmem, CL_TRUE, 0, local_NZ*N * sizeof(float), mtx.Value, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Col_clmem, CL_TRUE, 0, local_NZ*N * sizeof(int), mtx.Col, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Row_Index_clmem, CL_TRUE, 0, (N + 1) * sizeof(int), mtx.RowIndex, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Sv_clmem, CL_TRUE, 0, N * sizeof(float), Sv, 0, NULL, NULL);

	// Create a program from the kernel source
	//�������� ���������, ��������� ��������, ��� �������-����������
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);

	// Build the program
	//�������������� � �������� ���������
	//������ �������� - ���������; ������ -����� ���������; ������ - ������ ���������; ����� - If pfn_notify is NULL, clBuildProgram does not return until the build has completed.
	clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);
	
	// Create the OpenCL kernel
	//������ �������� - �������� �������, ����������� � ��������� ��� __kernel; 
	cl_kernel kernel[6];
	const char str[6][13] = { "square_fwd1", "square_fwd2" ,"square_y1" ,"square_y2", "square_x1","square_x2"};
	
	for (i = 0; i < 6; i++)
	{
		kernel[i]=InitKernel(program, str[i], clStatus);
	}

	// Set the arguments of the kernel
	SetKernel(kernel[0], Value_clmem, Col_clmem, Row_Index_clmem, koef_clmem);
	SetKernel(kernel[1], Value_clmem, Col_clmem, Row_Index_clmem, koef_clmem);
	//int m = 192 * ((N) / 192);
	size_t global_size=N;
	size_t local_size = 1; //N % 192; // Process the entire lists
	
	
	StartCounter();
	printf("START\n");
	//������ ��� ������ ����������
	for (i = 0; i < N; i++)
	{
		clStatus = clSetKernelArg(kernel[0], 3, sizeof(int), (void *)&i);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[0], 1, NULL, &global_size, &local_size, 0, NULL, NULL);
		printf("STATUS1= %d\n", clStatus);
		clStatus = clSetKernelArg(kernel[1], 3, sizeof(int), (void *)&i);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[1], 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
		printf("STATUS2= %d\n", clStatus);
	}

	SetKernel(kernel[2], X_clmem, Sv_clmem, koef_clmem);
	SetKernel(kernel[3], X_clmem, Sv_clmem, koef_clmem);

	for (i = 0; i < N; i++)
	{	
		clStatus = clSetKernelArg(kernel[2], 2, sizeof(int), (void *)&i);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[2], 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
		printf("STATUS_sqry1= %d\n", clStatus);

		clStatus = clSetKernelArg(kernel[3], 2, sizeof(int), (void *)&i);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[3], 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
		printf("STATUS_sqry2= %d\n", clStatus);
	}

	SetKernel(kernel[4], X_clmem, koef_clmem);
	SetKernel(kernel[5], X_clmem, koef_clmem);
	//�������� ��� ������ ����������
	for (int i = N-1; i >= 0; i--)
	{
		clStatus = clSetKernelArg(kernel[4], 1, sizeof(int), (void *)&i);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[4], 1, NULL, &global_size, NULL, 0, NULL, NULL);
		printf("STATUS_sqrx1= %d\n", clStatus);
		if (i > 0)
		{
			clStatus = clSetKernelArg(kernel[5], 1, sizeof(int), (void *)&i);
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel[5], 1, NULL, &global_size, NULL, 0, NULL, NULL);
			printf("STATUS_sqrx2= %d\n", clStatus);
		}
	}
	double time = GetCounter();
	clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, N * sizeof(float), X, 0, NULL, NULL);
	printf("SOLUTION\n");
	for (i = 0; i<N; i++)
	{
		printf("%d) %f\n-------\n", i+1,X[i]);
	}
	/*cout << "KOEF" << endl;
	for (i = 0; i<N; i++)
	{
		for (int j = 0; j<=i; j++)
		{
			cout << (((1 + i)*i) / 2) + j << ") " << koef[(((1 + i)*i) / 2)+j] << endl << "-------" << endl;
		} 
	}*/
	//MapleCheck(mtx,Sv);
	printf("GPU Time=%f sec.\n", time);
	
	/*int count = 1, sum=N, check=1;
	while (check<local_NZ)
	{
		sum += (N - count) * 2;
		check += 2;
		count++;
	}
	printf("NON ZERO= %f %%\n", (sum * 100.0) / (N*N));*/
		
//������� ������
#pragma region CleanUp
	// Clean up and wait for all the comands to complete.
	//��� ������� ��������� � �������
	clStatus = clFlush(command_queue);
	//���������� ���� ���������� ������
	clStatus = clFinish(command_queue);

	for (i = 0; i < 6; i++)
	{
		FreeKernel(kernel[i]);
	}

	clStatus = clReleaseProgram(program);
	clStatus = clReleaseCommandQueue(command_queue);
	clStatus = clReleaseContext(context);

	FreeMemory(mtx,X,Sv,koef, source_str, platforms, device_list, Value_clmem, Col_clmem, Row_Index_clmem);
#pragma endregion

#pragma endregion
	
	system("pause");
	return 0;
}