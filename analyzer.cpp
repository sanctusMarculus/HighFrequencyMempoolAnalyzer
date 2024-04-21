#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>


using namespace std;
using json = nlohmann::json;

size_t writeCallback_mempoolConn(void* contents, size_t size, size_t nmemb, string* userp) {
    size_t realsize = size * nmemb;
    userp->append((char*)contents, realsize);
    return realsize;
}

void processMatches(const json& transactions) {
    for (const auto& transaction : transactions.items()) {
        const string firstKey = transaction.key();
        const string hash = transaction.value().begin().value()["hash"];
        cout << "\n\nFrom: " << firstKey << "\nHash: " << hash << "\n";
        
    }
}

void monitorMempool(CURL* curl, struct curl_slist* headers, const string& data) {
    string readBuffer;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str()); 

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        try {
            json jsonResponse = json::parse(readBuffer);
            if (jsonResponse.contains("result")) {
                json result = jsonResponse["result"];

                if (result.contains("queued")) {
                    processMatches(result["queued"]);
                }

                if (result.contains("pending")) {
                    processMatches(result["pending"]);
                }
            }
        } catch (const json::parse_error& e) {
            return;
        }
    } else {
        cerr << "Failed: " << curl_easy_strerror(res) << endl;
    }
}

int main() {
    CURL* curl = curl_easy_init();
    struct curl_slist* headers = NULL;

    if (curl) {
        // Set constant configurations
        curl_easy_setopt(curl, CURLOPT_URL, "SERVER-ADDRESS");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback_mempoolConn);

        string data = R"(
            {
                "method": "txpool_content",
                "params": [],
                "id": 2,
                "jsonrpc": "2.0"
            }
        )";

        while (true) {
            monitorMempool(curl, headers, data); // Pass the 'headers' variable here
        }
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return 0;
}
