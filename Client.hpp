#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string>

class Client
{
private:
	int _fd;
	std::string _nickname;
	std::string _username;
	std::string _realname;
	bool _isRegistered;
	bool _isAuthenticated;
	std::string _buffer;
	bool _shouldQuit;
	std::string _quitMessage;
	bool _capPending;    // true while CAP negotiation is in progress

public:
	Client(int fd, const std::string &nickname, const std::string &username, const std::string &realname);
	~Client();

	// Getters
	int getFd() const;
	std::string getNickname() const;
	std::string getUsername() const;
	std::string getRealname() const;
	bool isRegistered() const;
	bool isAuthenticated() const;
	std::string getBuffer() const;
	bool shouldQuit() const;
	std::string getQuitMessage() const;
	bool isCapPending() const;
	void setCapPending(bool val);

	// Setters
	void setNickname(const std::string &nickname);
	void setUsername(const std::string &username);
	void setRealname(const std::string &realname);
	void setRegistered(bool registered);
	void setAuthenticated(bool authenticated);
	void setShouldQuit(bool shouldQuit);
	void setQuitMessage(const std::string &quitMessage);

	// Buffer management
	void appendToBuffer(const std::string &data);
	void clearBuffer();
};

#endif