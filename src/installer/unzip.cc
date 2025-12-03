/**
 * ==================================================
 *   _____ _ _ _             _
 *  |     |_| | |___ ___ ___|_|_ _ _____
 *  | | | | | | | -_|   |   | | | |     |
 *  |_|_|_|_|_|_|___|_|_|_|_|_|___|_|_|_|
 *
 * ==================================================
 *
 * Copyright (c) 2025 Project Millennium
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <minizip/unzip.h>
#include <iostream>
#include <filesystem>
#include <thread>
#include <vector>

#define WRITE_BUFFER_SIZE 8192

/**
 * @brief Create directories that do not exist.
 * @note This function is used to create directories that do not exist when extracting a zip archive.
 *
 * @param path The path to create directories for.
 */
void CreateNonExistentDirectories(std::filesystem::path path)
{
    std::filesystem::path dir_path(path);
    try {
        std::filesystem::create_directories(dir_path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directories: " << e.what() << std::endl;
    }
}

/**
 * @brief Extract a zipped file to a directory.
 * @param zipfile The zip file to extract.
 * @param fileName The name of the file to extract.
 */
void ExtractZippedFile(unzFile zipfile, std::filesystem::path fileName)
{
    char buffer[WRITE_BUFFER_SIZE];
    FILE* outfile = fopen(fileName.string().c_str(), "wb");

    if (!outfile) {
        std::cerr << "Error opening file " << fileName.string() << " for writing" << std::endl;
        return;
    }

    int nBytesRead;

    do {
        nBytesRead = unzReadCurrentFile(zipfile, buffer, WRITE_BUFFER_SIZE);
        if (nBytesRead < 0) {
            std::cerr << "Error reading file " << fileName.string() << " from zip archive" << std::endl;
            fclose(outfile);
            return;
        }
        if (nBytesRead > 0) {
            fwrite(buffer, 1, nBytesRead, outfile);
        }
    } while (nBytesRead > 0);

    fclose(outfile);
}

/**
 * @brief Check if a path is a directory path.
 * @note This function does not check if an actual directory exists, it only checks if the path string is directory-like.
 */
bool IsDirectoryPath(const std::filesystem::path& path)
{
    return path.has_filename() == false || (path.string().back() == '/' || path.string().back() == '\\');
}

/**
 * @brief Extract a zipped archive to a directory.
 * @param zipFilePath The path to the zip file.
 * @param outputDirectory The directory to extract the zip file to.
 * @param overallProgress Pointer to a double to track overall progress (0-1 scale).
 * @param fileProgress Pointer to a double to track file progress (0-1 scale).
 * @note This function extracts a zip file to a specified directory and tracks the progress of the extraction.
 */
void ExtractZippedArchive(const char* zipFilePath, const char* outputDirectory, double* overallProgress, double* fileProgress)
{
    std::cout << "[unzip] Extracting zip file: " << zipFilePath << " to " << outputDirectory << std::endl;

    unzFile zipfile = unzOpen(zipFilePath);
    if (!zipfile) {
        std::cerr << "Error: Cannot open zip file " << zipFilePath << std::endl;
        return;
    }

    int totalFiles = 0;
    uLong totalSize = 0;

    if (unzGoToFirstFile(zipfile) == UNZ_OK) {
        do {
            unz_file_info zipedFileMetadata;
            std::vector<char> zStrFileName(4096);

            if (unzGetCurrentFileInfo(zipfile, &zipedFileMetadata, zStrFileName.data(), (uInt)zStrFileName.size(), NULL, 0, NULL, 0) == UNZ_OK) {
                totalFiles++;
                totalSize += zipedFileMetadata.uncompressed_size;
            }
        } while (unzGoToNextFile(zipfile) == UNZ_OK);
    }

    unzClose(zipfile);
    zipfile = unzOpen(zipFilePath);

    if (unzGoToFirstFile(zipfile) != UNZ_OK) {
        std::cerr << "Error: Cannot find the first file in " << zipFilePath << std::endl;
        unzClose(zipfile);
        return;
    }

    int currentFileIndex = 0;
    uLong processedSize = 0;

    if (overallProgress)
        *overallProgress = 0.0;
    if (fileProgress)
        *fileProgress = 0.0;

    do {
        std::vector<char> zStrFileName(4096);
        unz_file_info zipedFileMetadata;

        if (unzGetCurrentFileInfo(zipfile, &zipedFileMetadata, zStrFileName.data(), (uInt)zStrFileName.size(), NULL, 0, NULL, 0) != UNZ_OK) {
            std::cerr << "Error reading file info in zip archive" << std::endl;
            if (unzGoToNextFile(zipfile) != UNZ_OK)
                break;
            else
                continue;
        }

        const std::string debugFileName = std::string(zStrFileName.data());
        std::cout << "[unzip] processing file #" << (currentFileIndex + 1) << " name='" << debugFileName << "'\n";

        currentFileIndex++;
        const std::string strFileName = std::string(zStrFileName.data());
        auto fsOutputDirectory = std::filesystem::path(outputDirectory) / strFileName;

        std::cout << "[unzip] output path: '" << fsOutputDirectory.string() << "'\n";

        if (overallProgress && totalSize > 0) {
            *overallProgress = static_cast<double>(processedSize) / totalSize;
        }

        if (IsDirectoryPath(fsOutputDirectory)) {
            CreateNonExistentDirectories(fsOutputDirectory);
            std::cout << "[unzip] created directory: '" << fsOutputDirectory.string() << "'\n";
            continue;
        }

        if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
            const std::string strFileName = std::string(zStrFileName.data());
            std::cerr << "\nError opening file " << strFileName << " in zip archive" << std::endl;
            break;
        }

        std::cout << "[unzip] opened file inside zip: '" << strFileName << "'\n";

        CreateNonExistentDirectories(std::filesystem::path(fsOutputDirectory).parent_path());
        FILE* outputFile = fopen(fsOutputDirectory.string().c_str(), "wb");
        if (!outputFile) {
            std::cerr << "\nError: Cannot create output file " << fsOutputDirectory.string() << std::endl;
            unzCloseCurrentFile(zipfile);
            continue;
        }

        const int bufferSize = 8192;
        char buffer[bufferSize];
        int bytesRead = 0;
        uLong currentFileBytesRead = 0;

        if (fileProgress)
            *fileProgress = 0.0;

        while ((bytesRead = unzReadCurrentFile(zipfile, buffer, bufferSize)) > 0) {
            fwrite(buffer, 1, bytesRead, outputFile);
            currentFileBytesRead += bytesRead;

            if ((currentFileBytesRead % (WRITE_BUFFER_SIZE * 8)) == 0) {
                std::cout << "[unzip] reading '" << strFileName << "' bytes read=" << currentFileBytesRead << "\n";
            }

            if (fileProgress && zipedFileMetadata.uncompressed_size > 0) {
                *fileProgress = static_cast<double>(currentFileBytesRead) / zipedFileMetadata.uncompressed_size;
            }
        }

        if (bytesRead < 0) {
            std::cerr << "\nError reading contents of " << fsOutputDirectory.string() << " from zip archive" << std::endl;
            fclose(outputFile);
            unzCloseCurrentFile(zipfile);
            continue;
        }

        fclose(outputFile);
        processedSize += zipedFileMetadata.uncompressed_size;
        unzCloseCurrentFile(zipfile);
        std::cout << "[unzip] finished file '" << strFileName << "' processedSize=" << processedSize << "\n";
    } while (unzGoToNextFile(zipfile) == UNZ_OK);

    if (overallProgress)
        *overallProgress = 1.0;
    if (fileProgress)
        *fileProgress = 1.0;

    std::cout << "[unzip] extraction complete\n";
    unzClose(zipfile);
}