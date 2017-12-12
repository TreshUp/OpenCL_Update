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

#define N 11
//#define NZ 121
#define local_NZ 3
//#define VECTOR_SIZE 100000
#define MAX_SOURCE_SIZE (0x100000)

using namespace std;
struct crsMatrix
{
	// Массив номеров столбцов (размер NZ)
	int* Col;
	// Массив индексов строк (размер N + 1)
	int* RowIndex;
	// Массив значений (размер NZ)
	double* Value;
};

void InitializeMatrix(crsMatrix &mtx)
{
	mtx.Value = new double[local_NZ*N];
	mtx.Col = new int[local_NZ*N];
	mtx.RowIndex = new int[N + 1];
}
void FreeMemory(crsMatrix &mtx)
{
	delete[] mtx.Value;
	delete[] mtx.Col;
	delete[] mtx.RowIndex;
}
void Generate(crsMatrix &mtx)
{
	int* Num_Non_zero;
	Num_Non_zero = new int[N];
	for (int c = 0; c < N; c++)
	{
		Num_Non_zero[c] = local_NZ;
	}
	ofstream fout;
	fout.setf(ios::fixed);
	fout.precision(1);
	fout.open("MTX.txt", ios::out);
	int count = 0;
	mtx.RowIndex[0] = 0;
	//srand(time(NULL));
	int i,j = 0;
	for (i = 0; i < N; i++)
	{
		int k = 0;
		int	tmp = i;
		int flag = 1;
		while (k < Num_Non_zero[i])
		{
			if (flag == 1)
			{
				mtx.Value[j] = rand() % 5 + 1;
				fout << " " << mtx.Value[j];
				mtx.Col[j] = i;
				fout << "( " << mtx.Col[j] << " )";
				count++;
				j++;
				k++;
				flag = 0;
				Num_Non_zero[i]--;
				//tmp = i;
			}
			if (Num_Non_zero[i] != 0)
			{
				if ((N - 1 - tmp) <= local_NZ)
				{
					//
					if (i + local_NZ >= N) tmp = N - 1;
					else tmp = i + local_NZ;

				}
				else tmp = tmp + rand() % ((N - tmp) / 3) + 1;
				mtx.Value[j] = rand() % 5 + 1;
				fout << " " << mtx.Value[j];
				mtx.Col[j] = tmp;
				fout << "( " << mtx.Col[j] << " )";
				count++;
				Num_Non_zero[tmp]--;
				j++;
				k++;
			}
		}
		/*if (Num_Non_zero[i] == 0)
		{
			mtx.Value[j] = 0;
			fout << " " << mtx.Value[j];
			mtx.Col[j] = i;
			fout << "( " << mtx.Col[j] << " )";
		}*/
		fout << endl;
		mtx.RowIndex[i+1] = count;
	}
	fout << "end";
	fout.close();
}

void Len_Generate(crsMatrix &mtx)
{

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
	/*if (i = 0; i < N; i++)
	{
		if (mtx.Value[mtx.RowIndex[i]])
	}*/
}
void Maple_check(crsMatrix &mtx)
{
	float** Check = (float**)calloc(N+1, sizeof(float*));
	int k = 0;
	for (k = 0; k < (N + 1); k++)
	{
		Check[k]= (float*)calloc(N, sizeof(float));
	}
	//for (int i = 0; i < N; i++)
	//{
		int i = 0;
		while (i < N)
		{
			int position = mtx.RowIndex[i];
			//for (int j = 0; j < N; j++)
			//{
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
		//}
	//}
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
	//Выделение памяти
	InitializeMatrix(mtx);

	Generate(mtx);
	Len_Generate(mtx);
	/*for (int i = 0; i < N + 1; i++)
	{
		cout<<mtx.RowIndex[i]<<endl;
	}*/
#pragma region OpenCL
#pragma region READ
	//Это все для чтения кернела из файла Hello.cl
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
		source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);//считывает массив размером MAX_SOURCE_SIZE, каждый размеров  1 // возвращает число успешно считанных элементов
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
	float s[1];
	//cout << "GET AND SET PLATFORM" << endl;
	// Get platform and device information
	cl_platform_id * platforms = NULL; //массив найденных платформ
	cl_uint num_platforms; //кол-во платформ

	//Set up the Platform
	cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms); //первый параметр - число платформ на "add"; на выходе в num_platforms записано число платформ
	platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms); //динамическое выделение памяти
	clStatus = clGetPlatformIDs(num_platforms, platforms, NULL); //получение списка доступных платформ; на выходе в массиве содер. все платформы

	//Get the devices list and choose the device you want to run on
	cl_device_id *device_list = NULL; //массив найденных устройств
	cl_uint num_devices; //кол-во устройств
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices); //первый параметр - какая платформа; второй - тип; третий - число устройств на добавление; на выходе в num_devices записано число устройств
	device_list = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);//динамическое выделение памяти
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, num_devices, device_list, NULL); //получение списка доступных устройств; на выходе в массиве содер. все устройства

	//cout << "CREATE CONTEXT" << endl;
	// Create one OpenCL context for each device in the platform
	cl_context context;
	context = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &clStatus); //создание контекста
	// Create a command queue
	cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus); // создание очереди из команд для устройства;properties - список свойств очереди команд

	//cout << "CREATE MEMORY" << endl;
	// Create memory buffers on the device for each vector
    //выделение памяти для каждого из векторов
	cl_mem Value_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, local_NZ*N * sizeof(double), NULL, &clStatus);
	cl_mem Col_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, local_NZ*N * sizeof(int), NULL, &clStatus);
	cl_mem Row_Index_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, (N + 1) * sizeof(int), NULL, &clStatus);
	cl_mem X_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, N * sizeof(float), NULL, &clStatus);
	cl_mem Sv_clmem= clCreateBuffer(context, CL_MEM_READ_WRITE, N * sizeof(float), NULL, &clStatus);
	cl_mem koef_clmem= clCreateBuffer(context, CL_MEM_READ_WRITE, ((1 + N)*N) / 2 * sizeof(float), NULL, &clStatus);
	cl_mem s_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, 1* sizeof(float), NULL, &clStatus);
	//cout << "COPY MATRIX TO DEVICE" << endl;
	// Copy the Buffer A and B to the device
	//копирование векторов А и В из памяти хоста в буфер; CL_TRUE - вектор А мб использован и после вызова функции clEnqueueWriteBuffer
	//мб меньше копировать????
	clStatus = clEnqueueWriteBuffer(command_queue, Value_clmem, CL_TRUE, 0, local_NZ*N * sizeof(double), mtx.Value, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Col_clmem, CL_TRUE, 0, local_NZ*N * sizeof(int), mtx.Col, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Row_Index_clmem, CL_TRUE, 0, (N + 1) * sizeof(int), mtx.RowIndex, 0, NULL, NULL);

	//cout << "CREATE 2 PROGRAMMS" << endl;
	// Create a program from the kernel source
	//создание программы, используя контекст, код функции-вычисления
	cl_program program1 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);
	cl_program program2 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);
	cl_program program3 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);
	cl_program program4 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);
	cl_program program5 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);
	cl_program program6 = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);

	//cout << "BUILD PROGRAM" << endl;
	// Build the program
	//компилирование и линковка программы
	//первый параметр - программа; второй -число устройств; третий - список устройств; пятый - If pfn_notify is NULL, clBuildProgram does not return until the build has completed.
	clStatus = clBuildProgram(program1, 1, device_list, NULL, NULL, NULL);
	clStatus = clBuildProgram(program2, 1, device_list, NULL, NULL, NULL);
	clStatus = clBuildProgram(program3, 1, device_list, NULL, NULL, NULL);
	clStatus = clBuildProgram(program4, 1, device_list, NULL, NULL, NULL);
	clStatus = clBuildProgram(program5, 1, device_list, NULL, NULL, NULL);
	clStatus = clBuildProgram(program6, 1, device_list, NULL, NULL, NULL);
	
	//cout << "CREATE 2 KERNELS" << endl;
	// Create the OpenCL kernel
	//второй параметр - название функции, объявленной в программе как __kernel; 
	cl_kernel kernel_fwd1 = clCreateKernel(program1, "square_fwd1", &clStatus);
	cl_kernel kernel_fwd2 = clCreateKernel(program2, "square_fwd2", &clStatus);
	cl_kernel kernel_sqr_y1 = clCreateKernel(program2, "square_y1", &clStatus);
	cl_kernel kernel_sqr_y2 = clCreateKernel(program2, "square_y2", &clStatus);
	cl_kernel kernel_sqr_x1 = clCreateKernel(program2, "square_x1", &clStatus);
	cl_kernel kernel_sqr_x2 = clCreateKernel(program2, "square_x2", &clStatus);

	//cout << "ARGUMENTS" << endl;
	// Set the arguments of the kernel
	//clStatus = clSetKernelArg(kernel_fwd1, 0, sizeof(cl_mem), (void *)&Value_clmem);
	clStatus = clSetKernelArg(kernel_fwd1, 1, sizeof(cl_mem), (void *)&Col_clmem);
	clStatus = clSetKernelArg(kernel_fwd1, 2, sizeof(cl_mem), (void *)&Row_Index_clmem);


	//clStatus = clSetKernelArg(kernel_fwd2, 0, sizeof(cl_mem), (void *)&Value_clmem);
	clStatus = clSetKernelArg(kernel_fwd2, 1, sizeof(cl_mem), (void *)&Col_clmem);
	clStatus = clSetKernelArg(kernel_fwd2, 2, sizeof(cl_mem), (void *)&Row_Index_clmem);

	clStatus = clSetKernelArg(kernel_sqr_y1, 0, sizeof(cl_mem), (void *)&Value_clmem);
	clStatus = clSetKernelArg(kernel_sqr_y1, 1, sizeof(cl_mem), (void *)&Col_clmem);
	clStatus = clSetKernelArg(kernel_sqr_y1, 2, sizeof(cl_mem), (void *)&Row_Index_clmem);
	//clStatus = clSetKernelArg(kernel_sqr_y1, 3, sizeof(cl_mem), (void *)&X_clmem);
	//clStatus = clSetKernelArg(kernel_sqr_y1, 4, sizeof(cl_mem), (void *)&Sv_clmem);

	clStatus = clSetKernelArg(kernel_sqr_y2, 0, sizeof(cl_mem), (void *)&Value_clmem);
	clStatus = clSetKernelArg(kernel_sqr_y2, 1, sizeof(cl_mem), (void *)&Col_clmem);
	clStatus = clSetKernelArg(kernel_sqr_y2, 2, sizeof(cl_mem), (void *)&Row_Index_clmem);
	//clStatus = clSetKernelArg(kernel_sqr_y2, 3, sizeof(cl_mem), (void *)&X_clmem);
	//clStatus = clSetKernelArg(kernel_sqr_y2, 4, sizeof(cl_mem), (void *)&Sv_clmem);

	clStatus = clSetKernelArg(kernel_sqr_x1, 0, sizeof(cl_mem), (void *)&Value_clmem);
	clStatus = clSetKernelArg(kernel_sqr_x1, 1, sizeof(cl_mem), (void *)&Col_clmem);
	clStatus = clSetKernelArg(kernel_sqr_x1, 2, sizeof(cl_mem), (void *)&Row_Index_clmem);
	

	clStatus = clSetKernelArg(kernel_sqr_x2, 0, sizeof(cl_mem), (void *)&Value_clmem);
	clStatus = clSetKernelArg(kernel_sqr_x2, 1, sizeof(cl_mem), (void *)&Col_clmem);
	clStatus = clSetKernelArg(kernel_sqr_x2, 2, sizeof(cl_mem), (void *)&Row_Index_clmem);

	
	///size_t m;
	
	//int count = mtx.RowIndex[N];

	size_t global_size = N; // Process the entire lists
	size_t local_size = 1; // Process one item at a time
	
	//m = 192 * (N/ 192);
	//m = N * (count / N);
	cout << "START" << endl;
	for (i = 0; i < N; i++)
	{
		clStatus = clSetKernelArg(kernel_fwd1, 0, sizeof(cl_mem), (void *)&Value_clmem);
		clStatus = clSetKernelArg(kernel_fwd1, 3, sizeof(int), (void *)&i);
		clStatus = clSetKernelArg(kernel_fwd1, 4, sizeof(cl_mem), (void *)&koef_clmem);
		//ToDO local_size
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_fwd1, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
		cout << "STATUS1= "<<clStatus << endl;
		//cout << "DONE 1" << endl << "------" << endl;
		//TMP
		clStatus = clEnqueueReadBuffer(command_queue, koef_clmem, CL_TRUE, 0, ((1 + N)*N) / 2 * sizeof(float), koef, 0, NULL, NULL);
		//clStatus = clEnqueueReadBuffer(command_queue, a, CL_TRUE, 0, 1* sizeof(float), a, 0, NULL, NULL);
		//koef[3] = -1;

		//size_t tmp = 192;
		//size_t tmp = N;
		s[0] = 3;
		clStatus = clSetKernelArg(kernel_fwd2, 0, sizeof(cl_mem), (void *)&Value_clmem);
		clStatus = clSetKernelArg(kernel_fwd2, 3, sizeof(int), (void *)&i);
		clStatus = clSetKernelArg(kernel_fwd2, 4, sizeof(cl_mem), (void *)&koef_clmem);
		clStatus = clSetKernelArg(kernel_fwd2, 5, sizeof(cl_mem), (void *)&s_clmem);
		//ToDO
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_fwd2, 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
		//clStatus = clEnqueueNDRangeKernel(command_queue, kernel_fwd2, 1, (0, 0, 0), &m, &tmp, 0, NULL, NULL);
		
		//cout << "DONE 2" << endl << "------" << endl;
		clStatus = clEnqueueReadBuffer(command_queue, koef_clmem, CL_TRUE, 0, ((1 + N)*N) / 2 * sizeof(float), koef, 0, NULL, NULL);
		clStatus = clEnqueueReadBuffer(command_queue, s_clmem, CL_TRUE, 0, 1* sizeof(float), s, 0, NULL, NULL);
		cout << "STATUS2= " << clStatus << endl;

		//cout << s[0] << " S[i]"<<endl;

		// если не кратно 192
		/*if (N % 192)
		{
		//if (count % N) {
			size_t tmp_1 = N % 192;
			//size_t tmp_1 = count % N;
			clStatus = clSetKernelArg(kernel_fwd2, 3, sizeof(int), (void *)&i);
			//ToDO
			//clStatus = clEnqueueNDRangeKernel(command_queue, kernel_fwd2, 1, (m, 0, 0), &global_size, &local_size, 0, NULL, NULL);
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel_fwd2, 1, (m, 0, 0), &tmp_1, &tmp_1, 0, NULL, NULL); //WORKS

		cout << clStatus << endl;
		cout << "DONE NE192" << endl << "------" << endl;*/
	}
	//clStatus = clEnqueueWriteBuffer(command_queue, koef_clmem, CL_TRUE, 0, ((1 + N)*N) / 2 * sizeof(float), koef, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Sv_clmem, CL_TRUE, 0, N * sizeof(float), Sv, 0, NULL, NULL);
		for (i = 0; i < N; i++)
		{
			clStatus = clSetKernelArg(kernel_sqr_y1, 3, sizeof(cl_mem), (void *)&X_clmem);
			clStatus = clSetKernelArg(kernel_sqr_y1, 4, sizeof(cl_mem), (void *)&Sv_clmem);
			clStatus = clSetKernelArg(kernel_sqr_y1, 5, sizeof(int), (void *)&i);
			clStatus = clSetKernelArg(kernel_sqr_y1, 6, sizeof(cl_mem), (void *)&koef_clmem);
			//ToDO local_size
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel_sqr_y1, 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
			cout << "STATUS_sqry1= " << clStatus << endl;
			clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, N * sizeof(float), X, 0, NULL, NULL);
			//clStatus = clEnqueueNDRangeKernel(command_queue, kernel_fwd2, 1, (0, 0, 0), &m, &tmp, 0, NULL, NULL);

			clStatus = clSetKernelArg(kernel_sqr_y2, 3, sizeof(cl_mem), (void *)&X_clmem);
			clStatus = clSetKernelArg(kernel_sqr_y2, 4, sizeof(cl_mem), (void *)&Sv_clmem);
			clStatus = clSetKernelArg(kernel_sqr_y2, 5, sizeof(int), (void *)&i);
			clStatus = clSetKernelArg(kernel_sqr_y2, 6, sizeof(cl_mem), (void *)&koef_clmem);
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel_sqr_y2, 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
			cout << "STATUS_sqry_2= " << clStatus << endl;
			/*square_y1(buffA, xcl, n + 1, i);
			square_y2.setGlobalWorkSize(m);
			square_y2.setGlobalWorkOffset(0, 0, 0);
			square_y2.setLocalWorkSize(192);
			square_y2(buffA, xcl, n + 1, i);*/

			/*if (N % 192)
			{
				size_t tmp_1 = N % 192;
				//size_t tmp_1 = count % N;
				clStatus = clSetKernelArg(kernel_fwd2, 3, sizeof(int), (void *)&i);
				//ToDO
				//clStatus = clEnqueueNDRangeKernel(command_queue, kernel_fwd2, 1, (m, 0, 0), &global_size, &local_size, 0, NULL, NULL);
				clStatus = clEnqueueNDRangeKernel(command_queue, kernel_fwd2, 1, (m, 0, 0), &tmp_1, &tmp_1, 0, NULL, NULL); //WORKS
				/*square_y2.setGlobalWorkSize(n % 192);
				square_y2.setGlobalWorkOffset(m, 0, 0);
				square_y2.setLocalWorkSize(n % 192);
				square_y2(buffA, xcl, n + 1, i);
			}*/
		}
		for (int i = N-1; i >= 0; i--)
		{
			//square_x1(buffA, xcl, n + 1, i);
			clStatus = clSetKernelArg(kernel_sqr_x1, 3, sizeof(cl_mem), (void *)&X_clmem);
			clStatus = clSetKernelArg(kernel_sqr_x1, 4, sizeof(int), (void *)&i);
			clStatus = clSetKernelArg(kernel_sqr_x1, 5, sizeof(cl_mem), (void *)&koef_clmem);
			//ToDO local_size
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel_sqr_x1, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
			cout << "STATUS_sqrx_1= " << clStatus << endl;
			clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, N * sizeof(float), X, 0, NULL, NULL);
			if (i > 0)
			{
				/*square_x2.setGlobalWorkSize(i);
				square_x2(buffA, xcl, n + 1, i);*/
				clStatus = clSetKernelArg(kernel_sqr_x2, 3, sizeof(cl_mem), (void *)&X_clmem);
				clStatus = clSetKernelArg(kernel_sqr_x2, 4, sizeof(int), (void *)&i);
				clStatus = clSetKernelArg(kernel_sqr_x2, 5, sizeof(cl_mem), (void *)&koef_clmem);
				//ToDO local_size
				clStatus = clEnqueueNDRangeKernel(command_queue, kernel_sqr_x2, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
				cout << "STATUS_sqrx_2= " << clStatus << endl;
				clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, N * sizeof(float), X, 0, NULL, NULL);
			}
		}
	//}
	//clStatus = clEnqueueReadBuffer(command_queue, Value_clmem, CL_TRUE, 0, local_NZ*N * sizeof(float), mtx.Value, 0, NULL, NULL);
	clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, local_NZ*N * sizeof(float), X, 0, NULL, NULL);
	//clStatus = clEnqueueReadBuffer(command_queue, koef_clmem, CL_TRUE, 0, ((1 + N)*N) / 2.0 * sizeof(float), koef, 0, NULL, NULL);
	/*cout << "FINAL VYVOD" << endl;
	for (i = 0; i<mtx.RowIndex[N]; i++)
	{
		cout <<i+1<<") "<< mtx.Value[i]<<endl << "-------" << endl;
	}*/

	cout << "SOLUTION" << endl;
	for (i = 0; i<N; i++)
	{
		cout << i + 1 << ") " << X[i] << endl << "-------" << endl;
	}
	cout << "KOEF" << endl;
	for (i = 0; i<N; i++)
	{
		for (int j = 0; j<=i; j++)
		{
			cout << (((1 + i)*i) / 2) + j << ") " << koef[(((1 + i)*i) / 2)+j] << endl << "-------" << endl;
		} 
	}
	Maple_check(mtx);
	/*float tmp=0;
	for (i = 0; i < N; i++)
	{
		float sum = 0;
		for (int k = mtx.RowIndex[i]; k <=( mtx.RowIndex[i+1] - 1); k++)
		{
			sum += X[mtx.Col[k]] * mtx.Value[k];
			//if (k > mtx.RowIndex[i]) tmp += X[mtx.Col[k]] * mtx.Value[k];
		}
		cout << sum << endl;
	}*/
#pragma region CleanUp
	// Clean up and wait for all the comands to complete.
	//все команды добавлены в очередь
	clStatus = clFlush(command_queue);
	//выполнение всех оставшихся команд
	clStatus = clFinish(command_queue);

	clStatus = clReleaseKernel(kernel_fwd1);
	clStatus = clReleaseKernel(kernel_fwd2);
	clStatus = clReleaseProgram(program1);
	clStatus = clReleaseProgram(program2);
	clStatus = clReleaseMemObject(Value_clmem);
	clStatus = clReleaseMemObject(Col_clmem);
	clStatus = clReleaseMemObject(Row_Index_clmem);
	clStatus = clReleaseCommandQueue(command_queue);
	clStatus = clReleaseContext(context);
	free(mtx.Value);
	free(mtx.Col);
	free(mtx.RowIndex);
	free(platforms);
	free(device_list);
#pragma endregion

#pragma endregion
	
	//Очистка памяти
	system("pause");
	return 0;
}