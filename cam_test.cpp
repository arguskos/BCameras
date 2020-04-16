#include <stdio.h>
#include <iostream>
#include "bgapi2_genicam/bgapi2_genicam.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/video/video.hpp>
#include "opencv2/core/ocl.hpp"

using namespace BGAPI2;

int main()
{
    int returncode = 0;
    int camfound = 0;
    try
    {
        SystemList *systemList = SystemList::GetInstance();
        systemList->Refresh();

        for (BGAPI2::SystemList::iterator it_s = systemList->begin(); it_s != systemList->end() && camfound == 0; it_s++)
        {
            System *pSystem = it_s->second; // gige, usb3, ..
            pSystem->Open();

            InterfaceList *interfaceList = pSystem->GetInterfaces();
            interfaceList->Refresh(100);
            for (BGAPI2::InterfaceList::iterator it_i = interfaceList->begin(); it_i != interfaceList->end() && camfound == 0; it_i++)
            {
                Interface *pInterface = it_i->second;
                pInterface->Open();

                DeviceList *deviceList = pInterface->GetDevices();
                deviceList->Refresh(100);
                if (deviceList->size() > 0)
                {
                    Device *pDevice = deviceList->begin()->second;
                    pDevice->Open();
                    std::cout << pDevice->GetModel() << "(" << pDevice->GetSerialNumber() << ")" << std::endl;
                    pDevice->GetRemoteNode("TriggerMode")->SetString("Off");

                    //////////////// CAMERA SETUP ///////////////////
                    pDevice->GetRemoteNode("GevHeartbeatTimeout")->SetInt(60000); // 1 minute = 60000 msec
                    std::cout << "    GevHeartbeatTimeout: " << pDevice->GetRemoteNode("GevHeartbeatTimeout")->GetValue();
                    if (pDevice->GetRemoteNode("GevHeartbeatTimeout")->HasUnit() == true)
                    {
                        std::cout << " [" << pDevice->GetRemoteNode("GevHeartbeatTimeout")->GetUnit() << "] " << std::endl
                                  << std::endl;
                    }

                    pDevice->GetRemoteNode(SFNC_BALANCEWHITEAUTO)->SetString("Periodic");
                    pDevice->GetRemoteNode("autoBrightnessMode")->SetString("Active");
                    /////////////////////////////////////

                    DataStreamList *datastreamList = pDevice->GetDataStreams();
                    datastreamList->Refresh();
                    DataStream *pDataStream = datastreamList->begin()->second;
                    pDataStream->Open();

                    BGAPI2::BufferList *bufferList = pDataStream->GetBufferList();
                    Buffer *pBuffer = NULL;
                    for (int i = 0; i < 4; i++)
                    {
                        pBuffer = new BGAPI2::Buffer();
                        bufferList->Add(pBuffer);
                        pBuffer->QueueBuffer();
                    }

                    pDataStream->StartAcquisitionContinuous();
                    pDevice->GetRemoteNode("AcquisitionStart")->Execute();

                    Buffer *pBufferFilled = NULL;

                    BGAPI2::ImageProcessor *pImageProcessor = new BGAPI2::ImageProcessor();
                    BGAPI2::Image *pImage = NULL;
                    void *pBufferDst = NULL;
                    bo_uint64 bufferSizeDst = 0;

                    cv::VideoWriter cvVideoCreator;                    // Create OpenCV video creator
                    cv::Mat openCvImage;                               // create an OpenCV image
                    cv::String videoFileName = "openCvVideo.avi";      // Define video filename
                    cv::Size frameSize = cv::Size(1936 / 2, 1216 / 2); // Define video frame size
                    cvVideoCreator.open(videoFileName, cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), 20, frameSize, true);
                    //settign pixel format
                    try
                    {
                        // pTransferSelector->SetValue("Stream0");
                        // pTransferStart->Execute();
                        // for (int i = 0; i < 100; i++)
                        while (true)
                        {
                            pBufferFilled = pDataStream->GetFilledBuffer(1000); //timeout 1000 msec

                            if (pBufferFilled == NULL)
                            {
                                std::cout << "Error: Buffer Timeout after 1000 msec" << std::endl;
                            }
                            else if (pBufferFilled->GetIsIncomplete() == true)
                            {
                                std::cout << "Error: Image is incomplete" << std::endl;
                                // queue buffer again
                                pBufferFilled->QueueBuffer();
                            }
                            else
                            {
                                void *pRawBuffer = pBufferFilled->GetMemPtr();

                                BGAPI2::String sPixelFormat = pBufferFilled->GetPixelFormat();
                                const int width = (int)(pBufferFilled->GetWidth());
                                const int height = (int)(pBufferFilled->GetHeight());
                                auto sFilename = "kek.jpg";
                                std::cout << "Widtrh " << width << " height " << height << std::endl;
                                std::cout << "Format " << sPixelFormat << std::endl;
                                std::cout << " Image " << std::setw(5) << pBufferFilled->GetFrameID() << " received in memory address " << std::hex << pBufferFilled->GetMemPtr() << std::dec << std::endl;
                                if (pImageProcessor != NULL)
                                { // Convert image to BGR8 and save as jpeg

                                    if (pImage == NULL)
                                    {
                                        // Create new BGAPI2::Image object
                                        pImage = pImageProcessor->CreateImage(width, height, sPixelFormat, pRawBuffer,
                                                                              pBufferFilled->GetMemSize());
                                    }
                                    else
                                    {
                                        // Reinitialise existing BGAPI2:::Image object
                                        pImage->Init(width, height, sPixelFormat, pRawBuffer, pBuffer->GetMemSize());
                                    }

                                    const bo_uint64 size = pImage->GetTransformBufferLength("BGR8");
                                    if (bufferSizeDst < size)
                                    {
                                        // destination buffer too small
                                        pBufferDst = realloc(pBufferDst, size);
                                        bufferSizeDst = size;
                                    }

                                    // Convert to BGR8
                                    pImageProcessor->TransformImageToBuffer(pImage, "BGR8", pBufferDst, bufferSizeDst);
                                    cv::Mat img(height, width, CV_8UC3, pBufferDst);
                                    cv::namedWindow("OpenCV window: Cam");
                                    cv::imshow("OpenCV window : Cam", img);
                                    cv::waitKey(1);
                                }
                                pBufferFilled->QueueBuffer();
                            }
                        }
                        if (pImageProcessor != NULL)
                        {
                            delete pImageProcessor;
                            pImageProcessor = NULL;
                        }
                    }
                    catch (BGAPI2::Exceptions::IException &ex)
                    {
                        returncode = (returncode == 0) ? 1 : returncode;
                        std::cout << "ExceptionType:    " << ex.GetType() << std::endl;
                        std::cout << "ErrorDescription: " << ex.GetErrorDescription() << std::endl;
                        std::cout << "in function:      " << ex.GetFunctionName() << std::endl;
                    }
                    if (pDevice->GetRemoteNodeList()->GetNodePresent("AcquisitionAbort"))
                    {
                        pDevice->GetRemoteNode("AcquisitionAbort")->Execute();
                    }
                    pDevice->GetRemoteNode("AcquisitionStop")->Execute();
                    pDataStream->StopAcquisition();

                    bufferList->DiscardAllBuffers();
                    while (bufferList->size() > 0)
                    {
                        pBuffer = bufferList->begin()->second;
                        bufferList->RevokeBuffer(pBuffer);
                        delete pBuffer;
                    }
                    pDataStream->Close();
                    pDevice->Close();
                    camfound = 1;
                }
                pInterface->Close();
            }
            pSystem->Close();
        }
        if (camfound == 0)
        {
            std::cout << "no camera found on any system or interface." << std::endl;
        }
    }
    catch (BGAPI2::Exceptions::IException &ex)
    {
        returncode = (returncode == 0) ? 1 : returncode;
        std::cout << "Error in function: " << ex.GetFunctionName() << std::endl
                  << "Error description: " << ex.GetErrorDescription() << std::endl
                  << std::endl;
    }
    BGAPI2::SystemList::ReleaseInstance();
    std::cout << "Input any number to close the program:";
    int endKey = 0;
    std::cin >> endKey;
    return returncode;
}
