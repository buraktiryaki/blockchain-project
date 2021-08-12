#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 300

#include <iostream>
#include <fstream>
#include <cerrno>
#include <string>

#include <CL/opencl.hpp>

#include "picosha2.h"

using namespace std;

#define MAX_THREAD_COUNT 3200
#define MAX_STR_SIZE 2048
#define MAX_BLOCK_COUNT 12

string get_file_contents(const char *filename);
cl::Device getDevice();

int main(int argc, char const *argv[])
{
    cl::Device device = getDevice();
    cout << "-Selected device: " << device.getInfo<CL_DEVICE_NAME>().c_str() << endl;

    string src = get_file_contents("../../kernel.ocl");

    cl::Context context(device);
    cl::Program program(context, src);

    try
    {
        program.build("-cl-std=CL2.0");
    }
    catch (...)
    {
        // Print build info for all devices
        cl_int buildErr = CL_SUCCESS;
        auto buildInfo = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);
        for (auto &pair : buildInfo)
        {
            std::cerr << pair.second << std::endl
                      << std::endl;
        }
        exit(1);
    }

    string strBuff = "Bize her yer Trabzon! Bölümün en yakışıklı hocası İbrahim Hoca’dır";
    string hash = "0000000000000000000000000000000000000000000000000000000000000000";
    for (int i = 1; i <= MAX_BLOCK_COUNT; i++)
    {
        cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
        cl::Buffer inBuff(context, CL_MEM_READ_WRITE, sizeof(cl_char) * MAX_STR_SIZE);
        cl::Buffer outBuff(context, CL_MEM_READ_WRITE, sizeof(cl_ulong));
        cl::Buffer boolBuff(context, CL_MEM_READ_WRITE, sizeof(cl_int));
        cl::Kernel kernel(program, "den");
        kernel.setArg(0, inBuff);
        kernel.setArg(1, outBuff);
        kernel.setArg(2, boolBuff);

        strBuff.insert(0, to_string(i));
        strBuff += hash;
        if (strBuff.size() >= MAX_STR_SIZE - 100)
        {
            cout << "string size exceeded..." << endl;
            exit(-1);
        }

        kernel.setArg(3, (cl_uint)strBuff.size());
        kernel.setArg(4, (cl_uint)i);
        queue.enqueueWriteBuffer(inBuff, CL_TRUE, 0, strBuff.size(), strBuff.data());
        cl_int boolVar = 1; //true
        queue.enqueueWriteBuffer(boolBuff, CL_TRUE, 0, sizeof(cl_int), &boolVar);

        cl::Event event;
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(MAX_THREAD_COUNT), cl::NullRange, 0, &event);
        queue.finish();
        cl_ulong start = event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        cl_ulong end = event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
        cout << i << ". block found" << endl;
        cout << "Elapsed time: " << double(end - start) / 1E6 << " ms" << endl;

        auto timenow =
            chrono::system_clock::to_time_t(chrono::system_clock::now());
        cout << "System Time: " << ctime(&timenow) << endl;

        cl_ulong output;
        queue.enqueueReadBuffer(outBuff, CL_TRUE, 0, sizeof(cl_ulong), &output);

        //combine nonce then find sha256 hash
        strBuff += to_string(output);
        std::vector<unsigned char> hashArr(picosha2::k_digest_size);
        picosha2::hash256(strBuff.begin(), strBuff.end(), hashArr.begin(), hashArr.end());
        hash = picosha2::bytes_to_hex_string(hashArr.begin(), hashArr.end());
        cout << "Full String: " << strBuff << endl;
        cout << "Nonce: " << output << endl;
        cout << "Hash: " << hash << endl;
        cout << "----------------------------------------------------------------\n\n";
    }

    cout << "end..." << endl;
    getchar();
    return 0;
}

cl::Device getDevice()
{
    vector<cl::Platform> platforms;
    vector<cl::Device> devices;
    cl::Platform::get(&platforms);
    if (platforms.size() < 1)
    {
        cout << "platform err." << endl;
        exit(-1);
    }
    platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &devices);
    if (devices.size() < 1)
    {
        cout << "device err." << endl;
        exit(-1);
    }
    return devices.front();
}

string get_file_contents(const char *filename)
{
    ifstream in(filename);
    if (in)
    {
        string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return contents;
    }
    else
    {
        cout << "file err..." << endl;
        exit(-1);
    }
}