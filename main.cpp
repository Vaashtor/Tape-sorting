#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <cmath>

using namespace std;
namespace fs = std::filesystem;

// какой размер ленты использовать для ленты размера size, чтобы размер был типа RAM * 2^n
int check_depth(int size, unsigned long long int RAM)
{
    int count = 1, tmp = RAM / sizeof(int);
    while (tmp < size)
    {
        tmp *= 2;
        count++;
    }
    return count;
}

//интерфейс для работы с лентой
class TapeInterface 
{
    public:
        virtual unsigned int read() = 0;
        virtual void write(int value) = 0;
        virtual void rewind() = 0;
        virtual void move_forward() = 0;
        virtual void move_backward() = 0;
        virtual void engage(string location) = 0;
        virtual unsigned int get_size() = 0;
};

//количество строк в файле
unsigned int count_lines(string FileName) 
{
    ifstream file(FileName);
    if (!file.is_open()) 
    {
        cout << "Ошибка открытия файла " << FileName << endl;
        return 0;
    }
    string line;
    unsigned int count = 0;
    while (getline(file, line)) 
    {
        count++;
    }
    if (line == "")
    {
        count ++;
    }
    file.close();
    return count;
}

// чтение строки № n из файла
unsigned int read_value_n(string FileName, unsigned int n)
{
    // вход: имя файла, номер строки, которую надо прочитать 
    ifstream file(FileName);
    if (!file.is_open()) 
    {
        cout << "Ошибка открытия файла " << FileName << endl;
        return 0;
    }
    unsigned int value;
    // читаем n строк, чтобы в переменной line осталась только строка № n
    for (unsigned int i = 0; i <= n; i++) 
    {
        file >> value;
    }
    file.close();
    // выход: строка № n
    return value;
};

// изменение строки № n в файле
void modify_value_n(string FileName, unsigned int n, unsigned int new_Value, unsigned int size) 
{
    ifstream inputFile(FileName);
    ofstream outputFile(FileName + ".tmp");
    if (!inputFile.is_open()) 
    {
        cout << "Ошибка открытия файла " << FileName << endl;
        return;
    }
    if (!outputFile.is_open()) 
    {
        cout << "Ошибка открытия файла " << FileName + ".tmp" << endl;
        return;
    }
    string line;

    // записываем все строки из старого файла в новый с измененной строкой № n
    for(unsigned int i = 0; i < size; i++) 
    {
        getline(inputFile, line);
        if (i == n)
        {
            outputFile << new_Value;
        }
        else
        {
            outputFile << line;
        }
        if (i != size - 1)
        {
            outputFile << "\n";
        }
    }

    inputFile.close();
    outputFile.close();
    fs::remove(FileName);
    fs::rename(FileName + ".tmp", FileName);
}

//Класс для описания работы ленты
class Tape: public TapeInterface 
{
    static chrono::milliseconds rwDelay;
    static chrono::milliseconds mDelay;
    static chrono::milliseconds rewindDelay;
    unsigned int size = 0;
    unsigned int position = 0;
    public:
        string TapeLocation;
        unsigned int read() override
        {
           return read_value_n(TapeLocation, position);
            this_thread::sleep_for(chrono::milliseconds(rwDelay));
        }
        void write(int value) override
        {
            modify_value_n(TapeLocation, position, value, size);
            this_thread::sleep_for(chrono::milliseconds(rwDelay));
        }
        void rewind() override
        {
            if (position > 0)
            {
                position = 0;
                this_thread::sleep_for(chrono::milliseconds(rewindDelay));
            }
        }
        void move_forward() override
        {
            if (position < size - 1) 
            {
                position++;
                this_thread::sleep_for(chrono::milliseconds(mDelay));
            }
            else
            {
                position = size - 1;
                cout << "Лента размером " << size << " на конечной позиции" << endl;
            }
        }
        void move_backward() override
        {
            if (position > 0) 
            {
                position--;
                this_thread::sleep_for(chrono::milliseconds(mDelay));
            }
            else
            {
                position = 0;
                cout << "Лента размером " << size << " на начальной позиции" << endl;
            }
        }
        void engage(string location) override
        {
            TapeLocation = location;
            size = count_lines(TapeLocation);
        }
        unsigned int get_size() override
        {
            return size;
        }
        static void setDelays(int read_write_delay, int move_delay, int rewind_delay) 
        {
            rwDelay = chrono::milliseconds(read_write_delay);
            mDelay = chrono::milliseconds(move_delay);
            rewindDelay = chrono::milliseconds(rewind_delay);
        }
};

// объединение двух отсортированных входных лент в одну выходную
void merge_tapes(TapeInterface& inputTape1, TapeInterface& inputTape2, TapeInterface& outputTape, int inputTapeSize, int outputTapeSize) 
{
    inputTape1.rewind();
    inputTape2.rewind();
    outputTape.rewind();
    unsigned int tape1Position = 0, tape2Position = 0;
    unsigned int value1 = inputTape1.read(), value2 = inputTape2.read();
    unsigned int size1 = inputTapeSize, size2 = outputTapeSize;
    while (true) 
    {
        // Если одна лента закончилась, выгружаем значения из оставшейся
        if (size1 == tape1Position) 
        {
            outputTape.write(value2);
            tape2Position ++;
            if (size2 == tape2Position) break; 
            inputTape2.move_forward();
        } 
        else if (size2 == tape2Position) 
        {
            outputTape.write(value1);
            tape1Position ++;
            if (size1 == tape1Position) break;
            inputTape1.move_forward();
        } 

        // Если обе ленты не закончились, записываем в выходную ленту наименьшее значение
        else 
        {
            if (value1 < value2) 
            {
                outputTape.write(value1);
                tape1Position ++;
                if (size1 != tape1Position) inputTape1.move_forward();
            } 
            else 
            {
                outputTape.write(value2);
                tape2Position ++;
                if (size2 != tape2Position) inputTape2.move_forward();
            }
        }
        value1 = inputTape1.read();
        value2 = inputTape2.read();

        outputTape.move_forward();
    }
}

// объединение двух отсортированных массивов в один
void merge_arrays(unsigned int array[], unsigned int const left, unsigned int const mid, unsigned int const right)
{
    unsigned int const LArr_len = mid - left + 1;
    unsigned int const RArr_len = right - mid;
    
    // создаем 2 временных массива
    unsigned int *leftArray = new unsigned int[LArr_len],
         *rightArray = new unsigned int[RArr_len];
 
    // заполняем временные массивы данными с левой и правой половины входного массива
    for (unsigned int i = 0; i < LArr_len; i++) leftArray[i] = array[left + i];
    for (unsigned int j = 0; j < RArr_len; j++) rightArray[j] = array[mid + 1 + j];
    
    unsigned int LArr_index = 0, RArr_index = 0; 
    unsigned int Output_index = left; 
    
    // Выбираем меньший элемент из левого и правого массивов и записываем его обратно во входной массив
    while (LArr_index < LArr_len && RArr_index < RArr_len) 
    {
        if (leftArray[LArr_index] <= rightArray[RArr_index]) 
        {
            array[Output_index] = leftArray[LArr_index];
            LArr_index++;
        }
        else
        {
            array[Output_index] = rightArray[RArr_index];
            RArr_index++;
        }
        Output_index++;
    }
    
    // Копируем элементы из оставшегося подмассива во входной массив
    while (LArr_index < LArr_len) 
    {
        array[Output_index] = leftArray[LArr_index];
        LArr_index++;
        Output_index++;
    }
    while (RArr_index < RArr_len) 
    {
        array[Output_index] = rightArray[RArr_index];
        RArr_index++;
        Output_index++;
    }
}

// сортировка массива
void merge_sort_array(unsigned int array[], unsigned int const begin, unsigned int const end)
{
    if (begin == end)
        return; 
    unsigned int mid = (end + begin) / 2;
    merge_sort_array(array, begin, mid);
    merge_sort_array(array, mid + 1, end);
    merge_arrays(array, begin, mid, end);
}

// сортировка ленты
void merge_sort_tape(TapeInterface& inputTape, TapeInterface& outputTape, unsigned long long int RAM, unsigned int const begin, unsigned int const end, bool position) 
{    
    // если размер подленты меньше, чем оперативная память, то:
    if (end - begin + 1 <= RAM / sizeof(int)) 
    {
        unsigned int array[end - begin + 1];
        //1. читаем значения из ленты в массив
        for (unsigned int i = begin; i <= end; i++) 
        {
            array[i - begin] = inputTape.read();
            if (i != inputTape.get_size() - 1) inputTape.move_forward();
        }

        //2. сортируем массив
        merge_sort_array(array, 0, end - begin);

        //3. записываем значения из массива в ленту:

        // Если вся изначальная лента вошла в ОЗУ, то записываем значения из массива в ленту
        if (end - begin + 1 == inputTape.get_size())
        {
            for (unsigned int i = 0; i <= end - begin; i++) 
            {
                outputTape.write(array[i]);
                if (i != outputTape.get_size() - 1) outputTape.move_forward();
            }
        }

        // иначе загружаем данные из массива во временную ленту
        else 
        {
            // смотрим, какой размер ленты нужен
            fs::path tempDir = fs::current_path() / "tmp";
            int depth = check_depth(end - begin + 1, RAM);
            string tempFileName = to_string(depth);
            if (position) 
                tempFileName = "left_" + tempFileName + ".txt";
            else 
                tempFileName = "right_" + tempFileName + ".txt";

            // создаем файл для временной ленты, если его еще нет
            ifstream tempFile(tempDir / tempFileName);
            if (!tempFile.is_open()) 
            {
                tempFile.close();
                ofstream tempFile(tempDir / tempFileName);

                for (unsigned int i = 0; i < pow(2, depth - 1) * RAM / sizeof(int) - 1; i++)
                {
                    tempFile << "\n";
                }
            }
            tempFile.close();
            
            Tape tempTape;
            tempTape.engage((tempDir / tempFileName).generic_string());
            tempTape.rewind();

            for (unsigned int i = 0; i <= end - begin; i++)
            {
                tempTape.write(array[i]);
                if (i != tempTape.get_size() - 1) tempTape.move_forward();
            }
        }
    }

    // если же размер необходимой подленты больше, чем оперативная память, то разбиваем эту подленту на 2 маленькие и объединяем их
    else 
    {
        // создаем 2 подленты и сортируем их
        unsigned int mid = (end + begin) / 2;
        merge_sort_tape(inputTape, outputTape, RAM, begin, mid, true);
        merge_sort_tape(inputTape, outputTape, RAM, mid + 1, end, false);
        Tape leftTape, rightTape;
        leftTape.engage((fs::current_path() / "tmp" / ("left_" + to_string(check_depth(mid - begin + 1, RAM)) + ".txt")).generic_string());
        rightTape.engage((fs::current_path() / "tmp" / ("right_" + to_string(check_depth(end - mid, RAM)) + ".txt")).generic_string());
        
        // если это первое вхождение, то если объеденить левую и правую подленты, получится отсортированная конечная лента
        if (end - begin + 1 == inputTape.get_size())
        {
            merge_tapes(leftTape, rightTape, outputTape, mid - begin + 1, end - mid);
        }

        // Иначе мы создаем временную подленту и записываем туда объединенные левую и правую ленты
        else
        {
            // смотрим, какой размер ленты нужен
            fs::path tempDir = fs::current_path() / "tmp";
            int depth = check_depth(end - begin + 1, RAM);
            string tempFileName = to_string(depth);
            if (position) 
                tempFileName = "left_" + tempFileName + ".txt";
            else 
                tempFileName = "right_" + tempFileName + ".txt";

            // создаем файл для временной ленты, если его еще нет
            ifstream tempFile(tempDir / tempFileName);
            if (!tempFile.is_open()) 
            {
                tempFile.close();
                ofstream tempFile(tempDir / tempFileName);

                for (unsigned int i = 0; i < pow(2, depth - 1) * RAM / sizeof(int) - 1; i++)
                {
                    tempFile << "\n";
                }
            }
            tempFile.close();
            
            Tape tempTape;
            tempTape.engage((tempDir / tempFileName).generic_string());
            tempTape.rewind();

            merge_tapes(leftTape, rightTape, tempTape, mid - begin + 1, end - mid);
        }
    }
}


chrono::milliseconds Tape::rwDelay = chrono::milliseconds(0);
chrono::milliseconds Tape::mDelay = chrono::milliseconds(0);
chrono::milliseconds Tape::rewindDelay = chrono::milliseconds(0);

int main() 
{
    // создаем папку tmp в директории программы, если её еще нет
    fs::path tempDir = fs::current_path() / "tmp";
    if (!fs::exists(tempDir)) 
    {
        fs::create_directory(tempDir);
    }
    
    // задержки на чтение/запись, перемещение, перемотку в миллисекундах
    int rwDelay, mDelay, rewind_Delay;
    // память в байтах, int = 4 байта
    unsigned long long int RAM;
    ifstream config("config.txt");
    if (!config.is_open()) 
    {
        cout << "Ошибка открытия файла config.txt" << endl;
        return 1;
    }

    else
    {
        string line;
        // загружаем параметры задержек из config.txt
        while (getline(config, line))
        {
            if (line.find("read_write_Delay") != string::npos)
            {
                rwDelay = stof( line.substr(line.find("=") + 1));
            }
            if (line.find("move_Delay") != string::npos)
            {
                mDelay = stof(line.substr(line.find("=") + 1));
            }
            if (line.find("rewind_Delay") != string::npos)
            {
                rewind_Delay = stof(line.substr(line.find("=") + 1));
            }   
            if (line.find("RAM") != string::npos)
            {
                RAM = stoi(line.substr(line.find("=") + 1));
            }       
        }
        config.close();
    }

    Tape::setDelays(rwDelay, mDelay, rewind_Delay);
    
    // считываем имена входного и выходного файлов
    Tape InputTape, OutputTape;
    string inputFileName, outputFileName;
    cout << "Введите имя файла входной ленты:"; cin >> inputFileName;
    InputTape.engage(inputFileName); 
    cout << "Введите имя файла выходной ленты:"; cin >> outputFileName;
    OutputTape.engage(outputFileName); 
    
    merge_sort_tape(InputTape, OutputTape, RAM, 0, InputTape.get_size() - 1, true);
    
    return 0;
}