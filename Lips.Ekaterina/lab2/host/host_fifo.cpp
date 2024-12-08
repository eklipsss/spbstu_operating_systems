#include "../interface/host_interface.h"
#include "../connection/conn_fifo.h"
#include "../host/host.h"
#include "../book.h"


#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <vector>


bool pick_up_book(std::vector<Book>& books, const std::string& book_name, std::string client_name) {
    std::cout << "takeBook\n";
    for (auto& book : books) {
        if (book.name == book_name) {
            if (book.count > 0) {
                book.count--;
                std::cout << "Book taken: " << book_name << ", Client_name: " << client_name << "\n";
                return true;
            } else {
                std::cout << "Book not available: " << book_name << "\n";
                return false;
            }
        }
    }
    std::cout << "No such book in library: " << book_name << "\n";
    return false;
}


bool return_book(std::vector<Book>& books, const std::string& book_name, std::string client_name) {
    for (auto& book : books) {
        if (book.name == book_name) {
            book.count++;
            std::cout << "Book returned: " << book_name << ", Client_name: " << client_name << "\n";
            return true;
        }
    }
    std::cout << "No such book in library: " << book_name << "\n";
    return false;
}


bool Host::setup_conn() {
    std::cout << "setup_conn\n";
    pid_t pid = getpid();
    std::ofstream pid_file("build/host_pid.txt");
    if (!pid_file) {
        std::cout << "Failed to open PID file for writing";
        // QMessageBox::critical(this, "Error", "Failed to open PID file for writing");
        return false;
    }
    pid_file << pid;
    pid_file.close();

    std::string client_path = "/tmp/chat_client_fifo" + std::to_string(pid);
    std::string host_path = "/tmp/chat_host_fifo" + std::to_string(pid);

    client_conn = new ConnFifo(client_path, true);
    if (!client_conn->is_valid()) {
        // QMessageBox::critical(this, "Error", "Failed to open FIFO for writing");
        return false;
    }

    host_conn = new ConnFifo(host_path, true);
    if (!host_conn->is_valid()) {
        // QMessageBox::critical(this, "Error", "Failed to open FIFO for reading");
        return false;
    }
    return true;
};


void Host::write_to_client(std::string response) {
    if (client_conn && client_conn->is_valid()) {
        if (!client_conn->write(response)) {
            std::cout << "Failed to write response \n";
            return;
        }
        std::cout << "Success write: " << response << "\n";
        // sem_post(sem_write);
    }
};


void Host::read_from_client() {
    while (is_running) {
        if (host_conn && host_conn->is_valid()) {
            std::string message;
            const size_t max_size = 1024;
            
            if (host_conn->read(message, max_size)) {
                std::cout << "Success read: " << message << "\n";
                std::string state;
                std::string client_name;
                std::string book_name;
                std::string time;
                bool flag = true;
                if (message.rfind("TAKE ", 0) == 0) {
                    state = "[TAKE]";
                    std::cout << "Read TAKE: " << message << "\n";
                    // std::string book_name = msg.substr(5);
                    std::string str = message.substr(5);
                    int delimetr_pos = str.find("#");
                    book_name = str.substr(0, delimetr_pos);
                    str = str.substr(delimetr_pos + 1);
                    delimetr_pos = str.find("#");
                    client_name = str.substr(0, delimetr_pos);
                    time = str.substr(delimetr_pos + 1);
                    std::string response;
                    if (pick_up_book(books, book_name, client_name)){
                        response = "YES" + book_name + "#" + time;
                    }
                    else {
                        response = "NO" + book_name + "#" + time;
                        flag = false;
                    }
                    write_to_client(response);
                    std::cout << "Write: " << response << "\n";
                } 
                else if (message.rfind("RETURN ", 0) == 0) {
                    state = "[RETURN]";
                    std::cout << "Read RETURN: " << message << "\n";
                    // std::string book_name = msg.substr(7);
                    std::string str = message.substr(7);
                    int delimetr_pos = str.find("#");
                    book_name = str.substr(0, delimetr_pos);
                    str = str.substr(delimetr_pos + 1);
                    delimetr_pos = str.find("#");
                    client_name = str.substr(0, delimetr_pos);
                    time = str.substr(delimetr_pos + 1);
                    return_book(books, book_name, client_name); 
                    
                }

                window.update_books(books, state, book_name, client_name, time, flag);
            }
        }
        sleep(1);
    }
};


Host::Host(const std::vector<Book>& books_) : window(books_) {
    books = books_;
    setup_conn();
}


void read_wrap(Host& host){
    host.read_from_client();
}


int main(int argc, char* argv[]) {

    QApplication app(argc, argv);
    Host host(books);

    std::thread read_thread(read_wrap, std::ref(host));

    host.window.show();

    int res = app.exec();

    host.is_running = false;

    if (read_thread.joinable()) {
        read_thread.join();
    }
    
    return res;
};