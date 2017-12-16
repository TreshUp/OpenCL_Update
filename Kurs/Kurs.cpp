#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <math.h>

#define N 7
#define local_NZ 2
#define MAX_SOURCE_SIZE (0x100000)

using namespace std;
struct crsMatrix
{
	// ������ ������� �������� (������ NZ)
	int* Col;
	// ������ �������� ����� (������ N + 1)
	int* RowIndex;
	// ������ �������� (������ NZ)
	double* Value;
};

void InitializeMatrix(crsMatrix &mtx)
{

	mtx.Value= (double*)calloc(local_NZ*N, sizeof(double));
	mtx.Col = (int*)calloc(local_NZ*N, sizeof(int));
	mtx.RowIndex = (int*)calloc(N + 1, sizeof(int));
}
void FreeMemory(crsMatrix &mtx, cl_platform_id * platforms, cl_device_id *device_list, cl_mem Value_clmem, cl_mem Col_clmem, cl_mem Row_Index_clmem)
{

	free(mtx.Value);
	free(mtx.Col);
	free(mtx.RowIndex);
	free(platforms);
	free(device_list);
	clReleaseMemObject(Value_clmem);
	clReleaseMemObject(Col_clmem);
	clReleaseMemObject(Row_Index_clmem);
}

void Len_Generate(crsMatrix &mtx)
{
	srand(time(NULL));
	ofstream fout;
	fout.setf(ios::fixed);
	fout.precision(0);
	fout.open("MTX_New.txt", ios::out);
	int i, j = 0;
	mtx.RowIndex[0] = 0;
	for (i = 0; i < N - 1; i++)
	{
		int  Num_Non_zero = 1 + local_NZ / 2;
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
	mtx.RowIndex[N] = mtx.RowIndex[N - 1] + local_NZ / 2;
	mtx.Value[j] = rand() % 5 + 11;
	mtx.Col[j] = N - 1;
	int j1, j2, k;
	for (i = 0; i < N - 1; i++)
	{
		j1 = mtx.RowIndex[i];
		j2 = mtx.RowIndex[i + 1] - 1;
		k = i;
		while (k != 0)
		{
			fout << "-" << " ";
			k--;
		}
		for (int j = j1; j <= j2; j++)
		{
			fout << mtx.Value[j] << "( " << mtx.Col[j] << " ) ";
		}
		fout << endl;
	}
	j1 = mtx.RowIndex[N - 1];
	k = N - 1;
	while (k != 0)
	{
		fout << "-" << " ";
		k--;
	}
	fout << mtx.Value[j1] << "( " << mtx.Col[j] << " ) " << endl;
	fout.close();

}
void Maple_check(crsMatrix &mtx)
{
	float** Check = (float**)calloc(N+1, sizeof(float*));
	int k = 0;
	for (k = 0; k < (N + 1); k++)
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
			if (j > i) Check[j][i] = mtx.Value[position];
			position++;
			j++;
		}
		i++;
	}
	ofstream fout;
	fout.open("Sv.txt", ios::out);
	for (int i = 0; i < N; i++)
	{
		Check[i][N] = i + 1;
		fout << Check[i][N]<<endl;
	}
	fout.close();
	fout.setf(ios::fixed);
	fout.precision(0);
	fout.open("Maple.txt", ios::out);
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			fout << Check[i][j]<<" ";
		}
		fout << endl;
	}
	fout.close();
}
int main()
{
	crsMatrix mtx;
	//��������� ������
	InitializeMatrix(mtx);

	Len_Generate(mtx);

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
	float *Sv = (float*)calloc(N, sizeof(float));
	//float* koef = (float*)calloc(((1+N)*N)/2.0, sizeof(float*));
	float* koef = (float*)malloc(((1 + N)*N) / 2.0 * sizeof(float*));
	for (i = 0; i < N; i++)
	{
		Sv[i] = i + 1;
	}

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
	cl_mem Value_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, local_NZ*N * sizeof(double), NULL, &clStatus);
	cl_mem Col_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, local_NZ*N * sizeof(int), NULL, &clStatus);
	cl_mem Row_Index_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, (N + 1) * sizeof(int), NULL, &clStatus);
	cl_mem X_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, N * sizeof(float), NULL, &clStatus);
	cl_mem Sv_clmem= clCreateBuffer(context, CL_MEM_READ_WRITE, N * sizeof(float), NULL, &clStatus);
	cl_mem koef_clmem= clCreateBuffer(context, CL_MEM_READ_WRITE, ((1 + N)*N) / 2 * sizeof(float), NULL, &clStatus);

	// Copy the Buffer A and B to the device
	//����������� �������� � � � �� ������ ����� � �����; CL_TRUE - ������ � �� ����������� � ����� ������ ������� clEnqueueWriteBuffer
	//�� ������ ����������????
	clStatus = clEnqueueWriteBuffer(command_queue, Value_clmem, CL_TRUE, 0, local_NZ*N * sizeof(double), mtx.Value, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Col_clmem, CL_TRUE, 0, local_NZ*N * sizeof(int), mtx.Col, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Row_Index_clmem, CL_TRUE, 0, (N + 1) * sizeof(int), mtx.RowIndex, 0, NULL, NULL);

	// Create a program from the kernel source
	//�������� ���������, ��������� ��������, ��� �������-����������
	cl_program program1 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);
	cl_program program2 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);
	cl_program program3 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);
	cl_program program4 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);
	cl_program program5 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);
	cl_program program6 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);

	// Build the program
	//�������������� � �������� ���������
	//������ �������� - ���������; ������ -����� ���������; ������ - ������ ���������; ����� - If pfn_notify is NULL, clBuildProgram does not return until the build has completed.
	clStatus = clBuildProgram(program1, 1, device_list, NULL, NULL, NULL);
	clStatus = clBuildProgram(program2, 1, device_list, NULL, NULL, NULL);
	clStatus = clBuildProgram(program3, 1, device_list, NULL, NULL, NULL);
	clStatus = clBuildProgram(program4, 1, device_list, NULL, NULL, NULL);
	clStatus = clBuildProgram(program5, 1, device_list, NULL, NULL, NULL);
	clStatus = clBuildProgram(program6, 1, device_list, NULL, NULL, NULL);
	

	// Create the OpenCL kernel
	//������ �������� - �������� �������, ����������� � ��������� ��� __kernel; 
	cl_kernel kernel_fwd1 = clCreateKernel(program1, "square_fwd1", &clStatus);
	cl_kernel kernel_fwd2 = clCreateKernel(program2, "square_fwd2", &clStatus);
	cl_kernel kernel_sqr_y1 = clCreateKernel(program2, "square_y1", &clStatus);
	cl_kernel kernel_sqr_y2 = clCreateKernel(program2, "square_y2", &clStatus);
	cl_kernel kernel_sqr_x1 = clCreateKernel(program2, "square_x1", &clStatus);
	cl_kernel kernel_sqr_x2 = clCreateKernel(program2, "square_x2", &clStatus);


	// Set the arguments of the kernel
	clStatus = clSetKernelArg(kernel_fwd1, 0, sizeof(cl_mem), (void *)&Value_clmem);
	clStatus = clSetKernelArg(kernel_fwd1, 1, sizeof(cl_mem), (void *)&Col_clmem);
	clStatus = clSetKernelArg(kernel_fwd1, 2, sizeof(cl_mem), (void *)&Row_Index_clmem);
	clStatus = clSetKernelArg(kernel_fwd1, 4, sizeof(cl_mem), (void *)&koef_clmem);

	clStatus = clSetKernelArg(kernel_fwd2, 0, sizeof(cl_mem), (void *)&Value_clmem);
	clStatus = clSetKernelArg(kernel_fwd2, 1, sizeof(cl_mem), (void *)&Col_clmem);
	clStatus = clSetKernelArg(kernel_fwd2, 2, sizeof(cl_mem), (void *)&Row_Index_clmem);
	clStatus = clSetKernelArg(kernel_fwd2, 4, sizeof(cl_mem), (void *)&koef_clmem);

	size_t global_size = N; // Process the entire lists
	size_t local_size = 1; // Process one item at a time
	

	cout << "START" << endl;
	for (i = 0; i < N; i++)
	{
		//clStatus = clSetKernelArg(kernel_fwd1, 0, sizeof(cl_mem), (void *)&Value_clmem);
		clStatus = clSetKernelArg(kernel_fwd1, 3, sizeof(int), (void *)&i);
		//clStatus = clSetKernelArg(kernel_fwd1, 4, sizeof(cl_mem), (void *)&koef_clmem);
		//ToDO local_size
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_fwd1, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
		cout << "STATUS1= "<<clStatus << endl;
		//TMP
		clStatus = clEnqueueReadBuffer(command_queue, koef_clmem, CL_TRUE, 0, ((1 + N)*N) / 2 * sizeof(float), koef, 0, NULL, NULL);

		//clStatus = clSetKernelArg(kernel_fwd2, 0, sizeof(cl_mem), (void *)&Value_clmem);
		clStatus = clSetKernelArg(kernel_fwd2, 3, sizeof(int), (void *)&i);
		//clStatus = clSetKernelArg(kernel_fwd2, 4, sizeof(cl_mem), (void *)&koef_clmem);
		//ToDO
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_fwd2, 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);

		clStatus = clEnqueueReadBuffer(command_queue, koef_clmem, CL_TRUE, 0, ((1 + N)*N) / 2 * sizeof(float), koef, 0, NULL, NULL);
		cout << "STATUS2= " << clStatus << endl;
	}
	clStatus = clEnqueueWriteBuffer(command_queue, Sv_clmem, CL_TRUE, 0, N * sizeof(float), Sv, 0, NULL, NULL);
		for (i = 0; i < N; i++)
		{
			clStatus = clSetKernelArg(kernel_sqr_y1, 0, sizeof(cl_mem), (void *)&X_clmem);
			clStatus = clSetKernelArg(kernel_sqr_y1, 1, sizeof(cl_mem), (void *)&Sv_clmem);
			clStatus = clSetKernelArg(kernel_sqr_y1, 2, sizeof(int), (void *)&i);
			clStatus = clSetKernelArg(kernel_sqr_y1, 3, sizeof(cl_mem), (void *)&koef_clmem);
			//ToDO local_size
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel_sqr_y1, 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
			cout << "STATUS_sqry1= " << clStatus << endl;
			clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, N * sizeof(float), X, 0, NULL, NULL);

			clStatus = clSetKernelArg(kernel_sqr_y2, 0, sizeof(cl_mem), (void *)&X_clmem);
			clStatus = clSetKernelArg(kernel_sqr_y2, 1, sizeof(cl_mem), (void *)&Sv_clmem);
			clStatus = clSetKernelArg(kernel_sqr_y2, 2, sizeof(int), (void *)&i);
			clStatus = clSetKernelArg(kernel_sqr_y2, 3, sizeof(cl_mem), (void *)&koef_clmem);
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel_sqr_y2, 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
			cout << "STATUS_sqry_2= " << clStatus << endl;
		}
		for (int i = N-1; i >= 0; i--)
		{
			clStatus = clSetKernelArg(kernel_sqr_x1, 0, sizeof(cl_mem), (void *)&X_clmem);
			clStatus = clSetKernelArg(kernel_sqr_x1, 1, sizeof(int), (void *)&i);
			clStatus = clSetKernelArg(kernel_sqr_x1, 2, sizeof(cl_mem), (void *)&koef_clmem);
			//ToDO local_size
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel_sqr_x1, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
			cout << "STATUS_sqrx_1= " << clStatus << endl;
			clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, N * sizeof(float), X, 0, NULL, NULL);
			if (i > 0)
			{

				clStatus = clSetKernelArg(kernel_sqr_x2, 0, sizeof(cl_mem), (void *)&X_clmem);
				clStatus = clSetKernelArg(kernel_sqr_x2, 1, sizeof(int), (void *)&i);
				clStatus = clSetKernelArg(kernel_sqr_x2, 2, sizeof(cl_mem), (void *)&koef_clmem);
				//ToDO local_size
				clStatus = clEnqueueNDRangeKernel(command_queue, kernel_sqr_x2, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
				cout << "STATUS_sqrx_2= " << clStatus << endl;
				clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, N * sizeof(float), X, 0, NULL, NULL);
			}
		}
	clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, local_NZ*N * sizeof(float), X, 0, NULL, NULL);
	
	cout << "SOLUTION" << endl;
	for (i = 0; i<N; i++)
	{
		cout << i + 1 << ") " << X[i] << endl << "-------" << endl;
	}
	/*cout << "KOEF" << endl;
	for (i = 0; i<N; i++)
	{
		for (int j = 0; j<=i; j++)
		{
			cout << (((1 + i)*i) / 2) + j << ") " << koef[(((1 + i)*i) / 2)+j] << endl << "-------" << endl;
		} 
	}*/
	Maple_check(mtx);
#pragma region CleanUp
	// Clean up and wait for all the comands to complete.
	//��� ������� ��������� � �������
	clStatus = clFlush(command_queue);
	//���������� ���� ���������� ������
	clStatus = clFinish(command_queue);

	clStatus = clReleaseKernel(kernel_fwd1);
	clStatus = clReleaseKernel(kernel_fwd2);
	clStatus = clReleaseProgram(program1);
	clStatus = clReleaseProgram(program2);
	clStatus = clReleaseCommandQueue(command_queue);
	clStatus = clReleaseContext(context);

	FreeMemory(mtx, platforms, device_list, Value_clmem, Col_clmem, Row_Index_clmem);
#pragma endregion

#pragma endregion
	
	//������� ������
	system("pause");
	return 0;
}