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
#include <zlib.h>
#include <mz_compat.h>
#include <iostream>
#include <filesystem>
#include <vector>

#define WRITE_BUFFER_SIZE 8192

/**
 * @brief Create directories that do not exist.
 * @note This function is used to create directories that do not exist when extracting a zip archive.
 *
 * @param path The path to create directories for.
 */
bool CreateNonExistentDirectories(std::filesystem::path path)
{
    std::filesystem::path dir_path(path);
    try {
        std::filesystem::create_directories(dir_path);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directories: " << e.what() << std::endl;
        return false;
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
bool ExtractZippedArchive(const char* zipFilePath, const char* outputDirectory, double* overallProgress, double* fileProgress)
{
    std::cout << "[unzip] Extracting zip file: " << zipFilePath << " to " << outputDirectory << std::endl;

    unzFile zipfile = unzOpen(zipFilePath);
    if (!zipfile) {
        std::cerr << "Error: Cannot open zip file " << zipFilePath << std::endl;
        return false;
    }

    unz_global_info globalInfo;
    if (unzGetGlobalInfo(zipfile, &globalInfo) != UNZ_OK) {
        std::cerr << "Error: Cannot read zip global info from " << zipFilePath << std::endl;
        unzClose(zipfile);
        return false;
    }

    if (unzGoToFirstFile(zipfile) != UNZ_OK) {
        std::cerr << "Error: Cannot find the first file in " << zipFilePath << std::endl;
        unzClose(zipfile);
        return false;
    }

    int currentFileIndex = 0;
    int totalFiles = static_cast<int>(globalInfo.number_entry);
    bool success = true;

    if (overallProgress)
        *overallProgress = 0.0;
    if (fileProgress)
        *fileProgress = 0.0;

    do {
        std::vector<char> zStrFileName(4096);
        unz_file_info zipedFileMetadata;

        if (unzGetCurrentFileInfo(zipfile, &zipedFileMetadata, zStrFileName.data(), (uInt)zStrFileName.size(), NULL, 0, NULL, 0) != UNZ_OK) {
            std::cerr << "Error reading file info in zip archive" << std::endl;
            success = false;
            break;
        }

        const std::string strFileName = std::string(zStrFileName.data());
        std::cout << "[unzip] processing file #" << (currentFileIndex + 1) << " name='" << strFileName << "'\n";

        currentFileIndex++;
        auto fsOutputDirectory = std::filesystem::path(outputDirectory) / strFileName;

        std::cout << "[unzip] output path: '" << fsOutputDirectory.string() << "'\n";

        if (overallProgress && totalFiles > 0) {
            *overallProgress = static_cast<double>(currentFileIndex - 1) / totalFiles;
        }

        if (IsDirectoryPath(fsOutputDirectory)) {
            if (!CreateNonExistentDirectories(fsOutputDirectory)) {
                std::cerr << "Error: Failed to create directory " << fsOutputDirectory.string() << std::endl;
                success = false;
                break;
            }
            std::cout << "[unzip] created directory: '" << fsOutputDirectory.string() << "'\n";
            continue;
        }

        if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
            std::cerr << "\nError opening file " << strFileName << " in zip archive" << std::endl;
            success = false;
            break;
        }

        std::cout << "[unzip] opened file inside zip: '" << strFileName << "'\n";

        if (!CreateNonExistentDirectories(std::filesystem::path(fsOutputDirectory).parent_path())) {
            std::cerr << "Error: Failed to create parent directory for " << fsOutputDirectory.string() << std::endl;
            unzCloseCurrentFile(zipfile);
            success = false;
            break;
        }

        FILE* outputFile = fopen(fsOutputDirectory.string().c_str(), "wb");
        if (!outputFile) {
            std::cerr << "\nError: Cannot create output file " << fsOutputDirectory.string() << std::endl;
            unzCloseCurrentFile(zipfile);
            success = false;
            break;
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
            success = false;
            break;
        }

        fclose(outputFile);
        unzCloseCurrentFile(zipfile);
        std::cout << "[unzip] finished file '" << strFileName << "'\n";
    } while (unzGoToNextFile(zipfile) == UNZ_OK);

    if (overallProgress)
        *overallProgress = 1.0;
    if (fileProgress)
        *fileProgress = 1.0;

    std::cout << "[unzip] extraction " << (success ? "complete" : "failed") << "\n";
    unzClose(zipfile);
    return success;
}
