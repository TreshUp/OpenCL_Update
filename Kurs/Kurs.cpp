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

//Размерность разреженой матрицы
#define N 50
//Число ненулевых элементов
#define local_NZ 5
//Размер буфера
#define MAX_SOURCE_SIZE (0x100000)

using namespace std;
//Структура, состоящая из трех числовых полей
struct crsMatrix
{
	// Массив номеров столбцов (размер NZ)
	int* Col;
	// Массив индексов строк (размер N + 1)
	int* RowIndex;
	// Массив значений (размер NZ)
	float* Value;
};
//функция, в которой происходит первоначальная инициализация разреженной матрицы 
void InitializeMatrix(crsMatrix &mtx)
{
	//Динамическое выделение памяти
	mtx.Value = (float*)calloc((2*N - local_NZ/2.0)*(1+local_NZ/2.0)*0.5, sizeof(float));//(local_NZ*N, sizeof(float));
	mtx.Col = (int*)calloc((2 * N - local_NZ / 2.0)*(1 + local_NZ / 2.0)*0.5, sizeof(float));//(local_NZ*N, sizeof(int));
	mtx.RowIndex = (int*)calloc(N + 1, sizeof(int));
}
//Функция, очищающая и удаляющая все ранее выделенные динамические массивы.
void FreeMemory(crsMatrix &mtx,float* X, float* Sv, float* koef, char* str, cl_platform_id * platforms, cl_device_id *device_list, cl_mem Value_clmem, cl_mem Col_clmem, cl_mem Row_Index_clmem)
{
	//Освобождение динамические выделенной памяти
	free(mtx.Value);
	free(mtx.Col);
	free(mtx.RowIndex);
	free(X);
	free(Sv);
	free(koef);
	free(str);
	free(platforms);
	free(device_list);
	//Используется специальная функция clReleaseMemObject
	clReleaseMemObject(Value_clmem);
	clReleaseMemObject(Col_clmem);
	clReleaseMemObject(Row_Index_clmem);
}
//функция, очистка объекта типа cl_kernel с помощью функции clReleaseKernel
inline void FreeKernel(cl_kernel kernel)
{
	//Используется специальная функция clReleaseKernel
	cl_int clStatus = clReleaseKernel(kernel);
}
//Функция, передающая несколько входных параметров в kernel-функцию
void SetKernel(cl_kernel kernel, cl_mem Value_clmem, cl_mem Col_clmem, cl_mem Row_Index_clmem, cl_mem koef_clmem)
{
	cl_int clStatus;
	//Передаем буфер, ненулевых элементов
	clStatus = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&Value_clmem);
	//Передаем буфер, номеров столбцов ненулевых элементов
	clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&Col_clmem);
	//Передаем буфер, индексов начала всех строк
	clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&Row_Index_clmem);
	//Передаем буфер, коэффициентов LU разложения
	clStatus = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&koef_clmem);
}
void SetKernel(cl_kernel kernel, cl_mem X_clmem, cl_mem Sv_clmem, cl_mem koef_clmem)
{
	cl_int clStatus;
	//Передаем буфер, решений исходной разреженной матрицы
	clStatus = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&X_clmem);
	//Передаем буфер, состоящий из всех элементов столбца свободных членов
	clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&Sv_clmem);
	//Передаем буфер, коэффициентов LU разложения
	clStatus = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&koef_clmem);
}
void SetKernel(cl_kernel kernel, cl_mem X_clmem, cl_mem koef_clmem)
{
	cl_int clStatus;
	//Передаем буфер, решений исходной разреженной матрицы
	clStatus = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&X_clmem);
	//Передаем буфер, коэффициентов LU разложения
	clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&koef_clmem);
}
//функция, которая генерирует разреженную матрицу размера N на N с числом local_NZ ненулевых диагоналей
void LenGenerate(crsMatrix &mtx, float* Sv)
{
	srand(time(NULL));
	//инициализация счетчиков
	int i, j = 0;
	mtx.RowIndex[0] = 0;
	for (i = 0; i < N - 1; i++)
	{
		//число ненулевых элементов
		int  Num_Non_zero = 1 + local_NZ / 2;
		//Заполнение столбаца свободных членов
		Sv[i]= rand() % 100 + 1;
		mtx.RowIndex[i + 1] = mtx.RowIndex[i] + Num_Non_zero;
		int	tmp = i;
		while (Num_Non_zero != 0)
		{
			//Свойство диагонального преобладания
			if (tmp==i) mtx.Value[j] = rand() % 5 + 11;
			else mtx.Value[j] = rand() % 5 + 1;
			//Номер столбца
			mtx.Col[j] = tmp;
			//Уменьшение числа ненулевых элементов для данной строки
			Num_Non_zero--;
			//Изменение счетчиков
			tmp += 1;
			j++;
		}
	}
	//Заполнение крайних элементов
	Sv[N-1] = rand() % 100 + 1;
	mtx.RowIndex[N] = mtx.RowIndex[N - 1] + local_NZ / 2;
	mtx.Value[j] = rand() % 5 + 11;
	mtx.Col[j] = N - 1;
}
//Функция, осуществляющая запись исходной разреженной матрицы и столбца свободных членов в текстовые файлы
void MapleCheck(crsMatrix &mtx,float* Sv)
{
	float** Check = (float**)calloc(N, sizeof(float*));
	int k = 0;
	for (k = 0; k < N; k++)
	{
		Check[k]= (float*)calloc(N, sizeof(float));
	}
	int i = 0;
	//Создание временного массива для вывода в файл
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
	//Вывод в файл столбца свободных членов
	ofstream fout;
	fout.open("Sv.txt", ios::out);
	for (k = 0; k < N; k++)
	{
		fout << Sv[k]<<endl;
	}
	fout.close();
	fout.setf(ios::fixed);
	fout.precision(0);
	//Вывод в файл исходной разреженной матрицы
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
	//Освобождение памяти
	for (k = 0; k < N-2; k++)
	{
		free(Check[k]);
	}
	free(Check);
}
//Функция, в которой происходит создание и инициализация объектов типа cl_kernel
inline cl_kernel InitKernel(cl_program program, const char* str, cl_int clStatus)
{
	return clCreateKernel(program, str, &clStatus);
}


/*void StartCounter()
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
}*/
int main()
{
	//Инициализация переменных, используемых таймером
	double PCFreq = 0.0;
	__int64 CounterStart = 0;
	crsMatrix mtx;
	//Выделение памяти
	InitializeMatrix(mtx);
	float *Sv = (float*)calloc(N, sizeof(float));
	LenGenerate(mtx,Sv);

#pragma region OpenCL
#pragma region READ
	//Это все для чтения kernel-функций из файла Hello.cl
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
		//Считывает массив размером MAX_SOURCE_SIZE // Возвращает число успешно считанных элементов
		source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
		fclose(fp);
	}
	catch (int a) {
		printf("%f", a);
	}
#pragma endregion
	float *X = (float*)calloc(N, sizeof(float));
	float *koef = (float*)calloc(((1+N)*N)/2.0, sizeof(float*));

	// Получаем информации о присутсвующих платформах и устройствах
	//Массив найденных платформ
	cl_platform_id * platforms = NULL;
	//Количество платформ
	cl_uint num_platforms; 

	//Задание платформы
	//Первый параметр - число платформ на добавление; на выходе в num_platforms записано число платформ
	cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
	//Динамическое выделение памяти
	platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms);
	//Получение списка доступных платформ; на выходе в массиве содержатся все платформы
	clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

	//Получаем список всех устройств (CPU, GPU) и выбираем нужную нам
	//Массив найденных устройств
	cl_device_id *device_list = NULL; 
	//Количество устройств
	cl_uint num_devices;
	//Первый параметр - какая платформа; второй - тип; третий - число устройств на добавление; на выходе в num_devices записано число устройств
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
	//Динамическое выделение памяти
	device_list = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);
	//Получение списка доступных устройств; на выходе в массиве содержатся все устройства
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, num_devices, device_list, NULL); 

	//Создание OpenCL контекста для каждого из устройств на платформе
	cl_context context;
	//Создание контекста
	context = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &clStatus); 
	//Создание очереди из команд для устройства;properties - список свойств очереди команд
	cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus);


	//Создание буфферов
    //Выделение памяти на устройстве для каждого из массивов
	//Динамический буфер, в котором хранятся значения всех ненулевых элементов матрицы ((((N - local_NZ + 1) + N)*local_NZ) / 2.0, sizeof(float));
	cl_mem Value_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, /*local_NZ*N*/(2 * N - local_NZ / 2.0)*(1 + local_NZ / 2.0)*0.5 * sizeof(float), NULL, &clStatus);
	//Динамический буфер, в котором содержится номера столбцов всех ненулевых элементов
	cl_mem Col_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, /*local_NZ*N*/(2 * N - local_NZ / 2.0)*(1 + local_NZ / 2.0)*0.5 * sizeof(int), NULL, &clStatus);
	//Динамический буфер, в котором содержится индекс начала всех строк
	cl_mem Row_Index_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, (N + 1) * sizeof(int), NULL, &clStatus);
	//Динамический буфер, в котором содержится решение разреженной матрицы
	cl_mem X_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, N * sizeof(float), NULL, &clStatus);
	//Динамический буфер, в котором содержится столбец свободных членов
	cl_mem Sv_clmem= clCreateBuffer(context, CL_MEM_READ_WRITE, N * sizeof(float), NULL, &clStatus);
	//Динамический буфер, в котором содержится все коэффициенты разложения LU
	cl_mem koef_clmem= clCreateBuffer(context, CL_MEM_READ_WRITE, ((1 + N)*N) / 2 * sizeof(float), NULL, &clStatus);

	//Копирование буферов из памяти хоста на девайс; CL_TRUE - параметр, позволяющий использовать буфер и после вызова функции clEnqueueWriteBuffer

	clStatus = clEnqueueWriteBuffer(command_queue, Value_clmem, CL_TRUE, 0, /*local_NZ*N * sizeof(float)*/(2 * N - local_NZ / 2.0)*(1 + local_NZ / 2.0)*0.5 * sizeof(float), mtx.Value, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Col_clmem, CL_TRUE, 0, /*local_NZ*N * sizeof(int)*/ (2 * N - local_NZ / 2.0)*(1 + local_NZ / 2.0)*0.5 * sizeof(int), mtx.Col, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Row_Index_clmem, CL_TRUE, 0, (N + 1) * sizeof(int), mtx.RowIndex, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, Sv_clmem, CL_TRUE, 0, N * sizeof(float), Sv, 0, NULL, NULL);

	// Create a program from the kernel source
	//Создание программы, используя контекст, код функции-вычисления
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);

	// Build the program
	//Компилирование и линковка программы
	//первый параметр - программа; второй -число устройств; третий - список устройств; пятый - If pfn_notify is NULL, clBuildProgram does not return until the build has completed.
	clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);
	
	// Create the OpenCL kernel
	//Второй параметр - название функции, объявленной в программе как __kernel;
	//массив cl_kernel объектов
	cl_kernel kernel[6];
	//Массив всех названий kernel-functions
	const char str[6][13] = { "square_fwd1", "square_fwd2" ,"square_y1" ,"square_y2", "square_x1","square_x2"};
	//Для каждой из kernel-function вызывается функция инициализации
	for (i = 0; i < 6; i++)
	{
		kernel[i]=InitKernel(program, str[i], clStatus);
	}

	//Задание аргументов
	SetKernel(kernel[0], Value_clmem, Row_Index_clmem, koef_clmem);
	SetKernel(kernel[1], Value_clmem, Col_clmem, Row_Index_clmem, koef_clmem);
	
	SetKernel(kernel[2], X_clmem, Sv_clmem, koef_clmem);
	SetKernel(kernel[3], X_clmem, Sv_clmem, koef_clmem);
	
	SetKernel(kernel[4], X_clmem, koef_clmem);
	SetKernel(kernel[5], X_clmem, koef_clmem);
	
	//int m = 192 * ((N) / 192);
	//Задаем число work-items
	size_t global_size=N;
	//size_t local_size = 1;
	
	
	//StartCounter();
	//void StartCounter()
	//{
	//Инициализация таймера
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
			cout << "QueryPerformanceFrequency failed!\n";

	PCFreq = double(li.QuadPart);// / 1000.0;
	QueryPerformanceCounter(&li);
	//Запуск таймера
	CounterStart = li.QuadPart;
	//}
	printf("START\n");
	//Прямой ход метода Холесского
	for (i = 0; i < N; i++)
	{
		clStatus = clSetKernelArg(kernel[0], 2, sizeof(int), (void *)&i);
		//Запуск первого ядра прямого хода
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[0], 1, NULL, &global_size, NULL, 0, NULL, NULL);
		printf("STATUS1= %d\n", clStatus);
		//Запуск второго ядра прямого хода
		clStatus = clSetKernelArg(kernel[1], 3, sizeof(int), (void *)&i);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[1], 1, (0, 0, 0), &global_size, NULL, 0, NULL, NULL);
		printf("STATUS2= %d\n", clStatus);
	}
	//Задание аргументов
	
	//Обратный ход метода Холесского
	//Вычисление y
	for (i = 0; i < N; i++)
	{	
		clStatus = clSetKernelArg(kernel[2], 2, sizeof(int), (void *)&i);
		//Запуск третьего ядра обратного хода
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[2], 1, (0, 0, 0), &global_size, NULL, 0, NULL, NULL);
		printf("STATUS_sqry1= %d\n", clStatus);
		//Запуск четвертого ядра обратного хода
		clStatus = clSetKernelArg(kernel[3], 2, sizeof(int), (void *)&i);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[3], 1, (0, 0, 0), &global_size, NULL, 0, NULL, NULL);
		printf("STATUS_sqry2= %d\n", clStatus);
	}
	//Задание аргументов
	
	//Обратный ход метода Холесского
	//Вычисление искомого решения
	for (int i = N-1; i >= 0; i--)
	{
		//Запуск пятого ядра обратного хода
		clStatus = clSetKernelArg(kernel[4], 1, sizeof(int), (void *)&i);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel[4], 1, NULL, &global_size, NULL, 0, NULL, NULL);
		printf("STATUS_sqrx1= %d\n", clStatus);
		if (i > 0)
		{
			//Запуск шестого ядра обратного хода
			clStatus = clSetKernelArg(kernel[5], 1, sizeof(int), (void *)&i);
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel[5], 1, NULL, &global_size, NULL, 0, NULL, NULL);
			printf("STATUS_sqrx2= %d\n", clStatus);
		}
	}
	//double GetCounter()
	//{
		//LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		//return double(li.QuadPart - CounterStart) / PCFreq;
	//}
	//Время расчетов на GPU
	double time = double(li.QuadPart - CounterStart) / PCFreq;//= GetCounter();
	clStatus = clEnqueueReadBuffer(command_queue, X_clmem, CL_TRUE, 0, N * sizeof(float), X, 0, NULL, NULL);
	//Вывод полученного вектора на экран
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

	//Функция проверки высчислений
	MapleCheck(mtx,Sv);
	//Вывод времени
	printf("GPU Time=%f sec.\n", time);
	//Подсчет процентного содержания ненулевых элеменнтов
	int count = 1, sum=N, check=1;
	while (check<local_NZ)
	{
		sum += (N - count) * 2;
		check += 2;
		count++;
	}
	printf("NON ZERO= %f %%\n", (sum * 100.0) / (N*N));
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