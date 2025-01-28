//
// Created by Raphael Paquin on 2025-01-28.
// CLIENT CODE FOR UDP.
//

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    if (argc != 3) {
        std::cout << "ERROR : INCORRECT USAGE" << std::endl;
        std::cout << "Usage: `" << argv[0] << " <IP> <PORT>`" << std::endl;
        return -1;
    }

    const char *ip = argv[1];
    const char *port_str = argv[2];
    long int port = strtol(port_str, nullptr, 10);


    int sockfd;
    char buffer[BUFFER_SIZE];
    sockaddr_in serverAddr{};
    socklen_t addrLen = sizeof(serverAddr);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip); // Localhost

    while (true) {
        // Send message to server
        std::string message;
        std::cout << "Enter message to send: ";
        std::getline(std::cin, message);

        if (message == "exit") {
            std::cout << "Exiting..." << std::endl;
            break;
        }

        sendto(sockfd, message.c_str(), message.length(), 0, reinterpret_cast<sockaddr *>(&serverAddr), addrLen);

        // Receive acknowledgment from server
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, reinterpret_cast<struct sockaddr *>(&serverAddr), &addrLen);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }
        buffer[n] = '\0'; // Null-terminate the message
        std::cout << "Server response: " << buffer << std::endl;
    }

    close(sockfd);
    return 0;
}
