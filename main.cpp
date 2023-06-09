#include <iostream>
#include <vector>

// Mock Flash Memory Framework
class MockFlashMemory {
public:
    static const int PAGE_SIZE = 256;
    static const int TOTAL_PAGES = 256;

    MockFlashMemory() {
        memory.resize(PAGE_SIZE * TOTAL_PAGES, 0xFF);
    }

    void writePage(int page, const std::vector<uint8_t>& data) {
        if (page < 0 || page >= TOTAL_PAGES) {
            std::cout << "Invalid page number." << std::endl;
            return;
        }

        if (data.size() != PAGE_SIZE) {
            std::cout << "Invalid data size. Expected: " << PAGE_SIZE << " bytes." << std::endl;
            return;
        }

        for (size_t i = 0; i < data.size(); ++i) {
            memory[page * PAGE_SIZE + i] = data[i];
        }
    }

    std::vector<uint8_t> readPage(int page) {
        if (page < 0 || page >= TOTAL_PAGES) {
            std::cout << "Invalid page number." << std::endl;
            return std::vector<uint8_t>(PAGE_SIZE, 0xFF);
        }

        std::vector<uint8_t> data(memory.begin() + page * PAGE_SIZE, memory.begin() + (page + 1) * PAGE_SIZE);
        return data;
    }

    void erasePage(int page) {
        if (page < 0 || page >= TOTAL_PAGES) {
            std::cout << "Invalid page number." << std::endl;
            return;
        }

        std::fill(memory.begin() + page * PAGE_SIZE, memory.begin() + (page + 1) * PAGE_SIZE, 0xFF);
        std::cout << "Page " << page << " erased." << std::endl;
    }

    void printFirstPage() {
        std::cout << "First page of mock flash memory: ";
        for (int i = 0; i < PAGE_SIZE; ++i) {
            std::cout << std::hex << static_cast<int>(memory[i]) << " ";
        }
        std::cout << std::endl;
    }

private:
    std::vector<uint8_t> memory;
};

// Flash Memory Logger Module
class FlashMemoryLogger {
public:
    FlashMemoryLogger(MockFlashMemory& flashMemory) : flashMemory(flashMemory), logId(0), memoryRewrite(false) {}

    void writeLog(const std::vector<uint8_t>& logData) {
        if (logData.size() != LOG_SIZE) {
            std::cout << "Invalid log size. Expected: " << LOG_SIZE << " bytes." << std::endl;
            return;
        }
    
        int page = logId / LOGS_PER_PAGE;
        std::vector<uint8_t> pageData = flashMemory.readPage(page);
    
        int logOffset = (logId % LOGS_PER_PAGE) * LOG_SIZE;
        std::copy(logData.begin(), logData.end(), pageData.begin() + logOffset);
    
        // Update the log ID in the page data
        pageData[logOffset] = (logId >> 8) & 0xFF;
        pageData[logOffset + 1] = logId & 0xFF;
    
        flashMemory.writePage(page, pageData);
    
        if(logId < (LOG_MAX - 1)) {   //if existing memory is not filled with logs
            logId++;
        } else {
            logId = 0;
            memoryRewrite = true;
        }
        if(memoryRewrite && logId % LOGS_PER_PAGE == 0) {   //freeing up memory
                flashMemory.erasePage(logId / LOGS_PER_PAGE);
        }
    }

    std::vector<uint8_t> readLog(uint16_t logId) {
        int page = logId / LOGS_PER_PAGE;
        std::vector<uint8_t> pageData = flashMemory.readPage(page);
        std::vector<uint8_t> logData(pageData.begin() + (logId % LOGS_PER_PAGE) * LOG_SIZE,
                                     pageData.begin() + (logId % LOGS_PER_PAGE + 1) * LOG_SIZE);

        return logData;
    }

    void showMemoryUtilization() {
        int utilization = logId * 100 / (LOG_MAX);
        int blocks = logId;
        if(memoryRewrite) {  //if existing memory is full
            blocks = LOG_MAX; 
            utilization = 100;
        }
        std::cout << "Memory utilization in logs: " << std::dec << blocks << " (" << utilization << " %total)" << std::endl;
    }

private:
    MockFlashMemory& flashMemory;
    uint16_t logId;
    bool memoryRewrite = false;

    static const int LOG_SIZE = 8;  // 2-byte log ID + 6-byte log data
    static const int LOGS_PER_PAGE = MockFlashMemory::PAGE_SIZE / LOG_SIZE;
    static const int LOG_MAX = MockFlashMemory::TOTAL_PAGES * LOGS_PER_PAGE; // max number of logs
};

int main() {
    MockFlashMemory flashMemory;
    FlashMemoryLogger logger(flashMemory);

    flashMemory.printFirstPage();

    std::cout << "Filling memory with logs..." << std::endl;
    for (uint16_t i = 0; i < 8300; ++i) {
        std::vector<uint8_t> logData = {(i >> 8) & 0xFF, i & 0xFF, 1, 2, 3, 4, 5, 6};
        logger.writeLog(logData);
    }

    flashMemory.printFirstPage();

    // Access logs by ID 5 times
    for (int i = 0; i < 5; ++i) {
        std::vector<uint8_t> logData = logger.readLog(i);
        std::cout << "Log ID " << i << " data: ";
        for (const auto& byte : logData) {
            std::cout << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;
    }

    // Delete 3 first pages
    for (int i = 0; i < 3; ++i) {
        flashMemory.erasePage(i);
    }

    flashMemory.printFirstPage();

    // Test memory utilization
    logger.showMemoryUtilization();

    return 0;
}