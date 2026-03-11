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
	std::set<int> _inviteList;            // Invited fds (for +i)
	bool _inviteOnly;                     // +i mode
	bool _topicRestricted;                // +t mode
	std::string _key;                     // +k mode
	int _userLimit;                       // +l mode (0 = no limit)

public:
	Channel(const std::string &name);
	~Channel();

	// Getters
	std::string getName() const;
	std::string getTopic() const;
	const std::map<int, Client*> &getMembers() const;
	bool isOperator(int fd) const;
	bool hasMember(int fd) const;
	bool isInviteOnly() const;
	bool isTopicRestricted() const;
	const std::string &getKey() const;
	int getUserLimit() const;
	bool isInvited(int fd) const;

	// Setters
	void setTopic(const std::string &topic);
	void setInviteOnly(bool val);
	void setTopicRestricted(bool val);
	void setKey(const std::string &key);
	void setUserLimit(int limit);

	// Invite management
	void addInvite(int fd);
	void removeInvite(int fd);

	// Member management
	void addMember(int fd, Client *client, bool isOp = false);
	void removeMember(int fd);
	void promoteToOperator(int fd);
	void demoteFromOperator(int fd);
	size_t getMemberCount() const;
};

#endif
