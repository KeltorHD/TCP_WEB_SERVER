#include "tcpserver.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>

std::vector<std::string> delim(std::string str, std::string delim)
{
	std::vector<std::string> arr;
	size_t prev = 0;
	size_t next;
	size_t delta = delim.length();

	while ((next = str.find(delim, prev)) != std::string::npos)
	{
		arr.push_back(str.substr(prev, next - prev));
		prev = next + delta;
	}

	arr.push_back(str.substr(prev));

	return arr;
}

bool includes(std::string inc, std::string str)
{
	for (size_t i = 0; i < min(str.length(), inc.length()); i++)
	{
		if (inc[i] != str[i])
			return false;
	}
	return true;
}

int main()
{
	TCPServer server
	(
		80, 
		[](TCPServer::TCPClient& client)
		{
			std::string data{ client.get_data() };

			auto words{ delim(data, " ") };

			try
			{
				if (words.size() < 3)
					throw std::exception("501 Not Implemented");

				if (words[0] != "GET")
					throw std::exception("501 Not Implemented");

				if (words[2] != "HTTP/1.0")
					throw std::exception("501 Not Implemented");

				if (words[1] == "/" || words[1] == "/index.html")
				{
					std::string answer{ "HTTP/1.0 200 OK\r\n" };
					std::ifstream file("index.html");
					
					if (!file.is_open())
						throw std::exception("404 Not Found");
					
					std::string str(std::istreambuf_iterator<char>{file}, {});
					answer += "Content-Length: " + std::to_string(str.length()) + "\r\n\r\n\r\n";
					client.send_data(answer + str + "\r\n");
				}
				else if (includes(words[1], "/conflict/"))
				{
					std::string answer{ "HTTP/1.0 409 Conflict\r\n" };
					client.send_data(answer);
				}
				else if (includes(words[1], "/"))
				{
					std::string answer{ "HTTP/1.0 200 OK\r\n" };
					std::string f{};
					for (size_t i = 1; i < words[1].length(); i++)
					{
						f.push_back(words[1][i]);
					}
					std::ifstream file(f, std::ios::in | std::ios::binary);

					if (!file.is_open())
						throw std::exception("404 Not Found-f");

					char b;
					file >> b;
					std::cout << b << std::endl;

					std::string str(std::istreambuf_iterator<char>{file}, {});
					answer += "Content-Length: " + std::to_string(str.length()) + "\r\n\r\n\r\n";
					client.send_data(answer + str + "\r\n");
				}
				else
				{
					throw std::exception("404 Not Found");
				}
			}
			catch (const std::exception& e)
			{
				client.send_data(std::string("HTTP/1.0 ") + e.what() + std::string("\r\n"));
			}
		}
	);

	server.start();

	return 0;
}