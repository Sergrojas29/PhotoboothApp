#include <iostream>
#include <windows.h>  // Windows API
#include <EDSDK.h>    // Canon EDSDK

// Function to check EDSDK errors
void checkError(EdsError err, const char* message) {
    if (err != EDS_ERR_OK) {
        std::cerr << "Error: " << message << " (Code: " << std::hex << err << ")" << std::endl;
        exit(1);
    }
}

// Callback function to handle image download
EdsError EDSCALLBACK handleObjectEvent(EdsUInt32 event, EdsBaseRef object, EdsVoid* context) {
    if (event == kEdsObjectEvent_DirItemCreated) {
        std::cout << "New image detected!" << std::endl;

        EdsDirectoryItemRef dirItem = (EdsDirectoryItemRef)object;
        EdsStreamRef stream = nullptr;
        EdsError err;

        // Get file info
        EdsDirectoryItemInfo fileInfo;
        err = EdsGetDirectoryItemInfo(dirItem, &fileInfo);
        checkError(err, "Failed to get file info");

        // Create a file stream to save the image
        std::string filePath = "captured_image.jpg";
        err = EdsCreateFileStream(filePath.c_str(), kEdsFileCreateDisposition_CreateAlways, kEdsAccess_Write, &stream);
        checkError(err, "Failed to create file stream");

        // Download the file
        err = EdsDownload(dirItem, fileInfo.size, stream);
        checkError(err, "Failed to download image");

        err = EdsDownloadComplete(dirItem);
        checkError(err, "Failed to finalize download");

        std::cout << "Image saved as: " << filePath << std::endl;

        // Release resources
        EdsRelease(stream);
        EdsRelease(dirItem);
    }
    return EDS_ERR_OK;
}

int main() {
    EdsError err;
    EdsCameraListRef cameraList = nullptr;
    EdsCameraRef camera = nullptr;
    EdsUInt32 cameraCount = 0;

    // Step 1: Initialize the SDK
    err = EdsInitializeSDK();
    checkError(err, "Failed to initialize EDSDK");

    // Step 2: Get the list of connected cameras
    err = EdsGetCameraList(&cameraList);
    checkError(err, "Failed to get camera list");

    err = EdsGetChildCount(cameraList, &cameraCount);
    checkError(err, "Failed to get camera count");

    if (cameraCount == 0) {
        std::cout << "No Canon camera detected!" << std::endl;
        EdsRelease(cameraList);
        EdsTerminateSDK();
        return 1;
    }

    // Step 3: Get the first detected camera
    err = EdsGetChildAtIndex(cameraList, 0, &camera);
    checkError(err, "Failed to get camera reference");

    // Step 4: Open a session with the camera
    err = EdsOpenSession(camera);
    checkError(err, "Failed to open camera session");

    // Step 5: Set the camera to save images to the computer
    EdsUInt32 saveTarget = kEdsSaveTo_Host;
    err = EdsSetPropertyData(camera, kEdsPropID_SaveTo, 0, sizeof(saveTarget), &saveTarget);
    checkError(err, "Failed to set SaveTo property");

    // Step 6: Register the event handler to catch new images
    err = EdsSetObjectEventHandler(camera, kEdsObjectEvent_All, handleObjectEvent, nullptr);
    checkError(err, "Failed to register object event handler");

    std::cout << "Camera ready! Taking a picture..." << std::endl;

    // Step 7: Capture an image
    err = EdsSendCommand(camera, kEdsCameraCommand_TakePicture, 0);
    checkError(err, "Failed to capture image");

    // Wait to allow image download
    Sleep(3000);

    // Step 8: Close session and cleanup
    EdsCloseSession(camera);
    EdsRelease(camera);
    EdsRelease(cameraList);
    EdsTerminateSDK();

    std::cout << "Camera session closed. Image capture complete." << std::endl;
    return 0;
}
