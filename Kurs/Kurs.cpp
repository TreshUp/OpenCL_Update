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
	// Массив номеров столбцов (размер NZ)
	int* Col;
	// Массив индексов строк (размер N + 1)
	int* RowIndex;
	// Массив значений (размер NZ)
	double* Value;
};

void InitializeMatrix(crsMatrix &mtx)
{

	mtx.Value= (double*)calloc(local_NZ*N, sizeof(double));
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
	srand(time(NULL));
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
	Sv[N-1] = rand() % 5 + 1;
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
			if (j > i) Check[j][i] = mtx.Value[position];
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
	for (k = 0; k < N; k++)
	{
		free(Check[k]);
	}
	free(Check);
}
inline cl_kernel InitKernel(cl_program program, const char* str, cl_int clStatus)
{
	return clCreateKernel(program, str, &clStatus);
}
int main()
{
	crsMatrix mtx;
	//Выделение памяти
	InitializeMatrix(mtx);
	float *Sv = (float*)calloc(N, sizeof(float));
	LenGenerate(mtx,Sv);

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
	float *koef = (float*)calloc(((1+N)*N)/2.0, sizeof(float*));

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

	// Create one OpenCL context for each device in the platform
	cl_context context;
	context = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &clStatus); //создание контекста
	// Create a command queue
	cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus); // создание очереди из команд для устройства;properties - список свойств очереди команд


	// Create memory buffers on the device for each vector
    //выделение памяти для каждого из векторов
	cl_mem Value_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, local_NZ*N * sizeof(double), NULL, &clStatus);
	cl_mem Col_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, local_NZ*N * sizeof(int), NULL, &clStatus);
	cl_mem Row_Index_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, (N + 1) * sizeof(int), NULL, &clStatus);
	cl_mem X_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, N * sizeof(float), NULL, &clStatus);
	cl_mem Sv_clmem= clCreateBuffer(context, CL_MEM_READ_WRITE, N * sizeof(float), NULL, &clStatus);
	cl_mem koef_clmem= clCreateBuffer(context, CL_MEM_READ_WRITE, ((1 + N)*N) / 2 * sizeof(float), NULL, &clStatus);

	// Copy the Buffer A and B to the device
	//копирование векторов А и В из памяти хоста в буфер; CL_TRUE - вектор А мб использован и после вызова функции clEnqueueWriteBuffer

	clStatus = clEnqueueWriteBuffer(command_queue, Value_clmem, CL_TRUE, 0, local_NZ*N * sizeof(double), mtx.Value, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Col_clmem, CL_TRUE, 0, local_NZ*N * sizeof(int), mtx.Col, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Row_Index_clmem, CL_TRUE, 0, (N + 1) * sizeof(int), mtx.RowIndex, 0, NULL, NULL);

	// Create a program from the kernel source
	//создание программы, используя контекст, код функции-вычисления
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);

	// Build the program
	//компилирование и линковка программы
	//первый параметр - программа; второй -число устройств; третий - список устройств; пятый - If pfn_notify is NULL, clBuildProgram does not return until the build has completed.
	clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);
	
	// Create the OpenCL kernel
	//второй параметр - название функции, объявленной в программе как __kernel; 
	cl_kernel kernel[6];
	const char str[6][13] = { "square_fwd1", "square_fwd2" ,"square_y1" ,"square_y2", "square_x1","square_x2"};
	
	for (i = 0; i < 6; i++)
	{
		kernel[i]=InitKernel(program, str[i], clStatus);
	}

	// Set the arguments of the kernel
	SetKernel(kernel[0], Value_clmem, Col_clmem, Row_Index_clmem, koef_clmem);
	SetKernel(kernel[1], Value_clmem, Col_clmem, Row_Index_clmem, koef_clmem);

	size_t global_size = N; // Process the entire lists
	size_t local_size = 1; // Process one item at a time
	

	cout << "START" << endl;
	for (i = 0; i < N; i++)
	{
		clStatus = clSetKernelArg(kernel[0], 3, sizeof(int), (void *)&i);
		//ToDO local_size
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[0], 1, NULL, &global_size, &local_size, 0, NULL, NULL);
		cout << "STATUS1= "<<clStatus << endl;
		//TMP
		clStatus = clEnqueueReadBuffer(command_queue, koef_clmem, CL_TRUE, 0, ((1 + N)*N) / 2 * sizeof(float), koef, 0, NULL, NULL);
		clStatus = clSetKernelArg(kernel[1], 3, sizeof(int), (void *)&i);
		//ToDO
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[1], 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
		clStatus = clEnqueueReadBuffer(command_queue, koef_clmem, CL_TRUE, 0, ((1 + N)*N) / 2 * sizeof(float), koef, 0, NULL, NULL);
		cout << "STATUS2= " << clStatus << endl;
	}
	clStatus = clEnqueueWriteBuffer(command_queue, Sv_clmem, CL_TRUE, 0, N * sizeof(float), Sv, 0, NULL, NULL);

	SetKernel(kernel[2], X_clmem, Sv_clmem, koef_clmem);
	SetKernel(kernel[3], X_clmem, Sv_clmem, koef_clmem);

	for (i = 0; i < N; i++)
	{	
		clStatus = clSetKernelArg(kernel[2], 2, sizeof(int), (void *)&i);
		//ToDO local_size
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[2], 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
		cout << "STATUS_sqry1= " << clStatus << endl;
		clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, N * sizeof(float), X, 0, NULL, NULL);

		clStatus = clSetKernelArg(kernel[3], 2, sizeof(int), (void *)&i);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[3], 1, (0, 0, 0), &global_size, &local_size, 0, NULL, NULL);
		cout << "STATUS_sqry_2= " << clStatus << endl;
	}

	SetKernel(kernel[4], X_clmem, koef_clmem);
	SetKernel(kernel[5], X_clmem, koef_clmem);

	for (int i = N-1; i >= 0; i--)
	{
		clStatus = clSetKernelArg(kernel[4], 1, sizeof(int), (void *)&i);
		//ToDO local_size
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[4], 1, NULL, &global_size, &local_size, 0, NULL, NULL);
		cout << "STATUS_sqrx_1= " << clStatus << endl;
		clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, N * sizeof(float), X, 0, NULL, NULL);
		if (i > 0)
		{
			clStatus = clSetKernelArg(kernel[5], 1, sizeof(int), (void *)&i);
			//ToDO local_size
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel[5], 1, NULL, &global_size, &local_size, 0, NULL, NULL);
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
	MapleCheck(mtx,Sv);
//Очистка памяти
#pragma region CleanUp
	// Clean up and wait for all the comands to complete.
	//все команды добавлены в очередь
	clStatus = clFlush(command_queue);
	//выполнение всех оставшихся команд
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