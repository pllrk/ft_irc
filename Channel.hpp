#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <string>
#include <map>
#include <set>
#include "Client.hpp"

class Channel
{
private:
	std::string _name;
	std::string _topic;
	std::map<int, Client*> _members;      // fd -> Client pointer
	std::set<int> _operators;             // Set of operator fds

public:
	Channel(const std::string &name);
	~Channel();

	// Getters
	std::string getName() const;
	std::string getTopic() const;
	const std::map<int, Client*> &getMembers() const;
	bool isOperator(int fd) const;
	bool hasMember(int fd) const;

	// Setters
	void setTopic(const std::string &topic);

	// Member management
	void addMember(int fd, Client *client, bool isOp = false);
	void removeMember(int fd);
	void promoteToOperator(int fd);
	void demoteFromOperator(int fd);
	size_t getMemberCount() const;
};

#endif
