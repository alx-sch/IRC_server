#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// logUserAction, isValidChannelName
#include "../include/defines.hpp"	// color formatting

/**
Handles the IRC `MODE` command.

XXXXX

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the `MODE` command.
 @param tokens	Parsed IRC command tokens (e.g., {"MODE", "#channel", "+o", "user"}).

 @return		True if the command was processed successfully,
				false if an error occurred.
*/
bool Command::handleMode(Server* server, User* user, const std::vector<std::string>& tokens)
{
    if (!checkRegistered(user, "MODE"))
        return false;

    if (tokens.size() < 2)
    {
        logUserAction(user->getNickname(), user->getFd(), "sent MODE without parameters");
        user->replyError(461, "MODE", "Not enough parameters");
        return false;
    }

    const std::string& target = tokens[1];
    
    // Only handle channel modes for now (targets starting with #)
    if (target.empty() || target[0] != '#')
    {
        logUserAction(user->getNickname(), user->getFd(), "sent MODE for non-channel target: " + target);
        user->replyError(501, "", "Unknown MODE flag");
        return false;
    }

    Channel* channel = server->getChannel(target);
    if (!channel)
    {
        logUserAction(user->getNickname(), user->getFd(), "sent MODE for non-existing channel: " + target);
        user->replyError(403, target, "No such channel");
        return false;
    }

    if (!channel->is_user_member(user->getNickname()))
    {
        logUserAction(user->getNickname(), user->getFd(), "sent MODE for channel not a member of: " + target);
        user->replyError(442, target, "You're not on that channel");
        return false;
    }

    // MODE query: just show current modes
    if (tokens.size() == 2)
    {
        std::string modes = "+";
        std::string params = "";
        
        if (channel->is_invite_only())
            modes += "i";
        if (channel->has_topic_protection())
            modes += "t";
        if (channel->has_user_limit())
        {
            modes += "l";
            params += " " + toString(channel->get_user_limit());
        }
        if (channel->has_password())
        {
            modes += "k";
            params += " " + channel->get_password();
        }
        
        if (modes == "+")
            modes = "+";
        
        user->getOutputBuffer() += ":" + server->getServerName() + " 324 " 
                                  + user->getNickname() + " " + target 
                                  + " " + modes + params + "\r\n";
        
        logUserAction(user->getNickname(), user->getFd(), "queried modes for " + target + ": " + modes + params);
        return true;
    }

    // MODE change: requires operator privileges
    if (!channel->is_user_operator(user->getNickname()))
    {
        logUserAction(user->getNickname(), user->getFd(), "tried to change modes for " + target + " but is not an operator");
        user->replyError(482, target, "You're not channel operator");
        return false;
    }

    return handleModeChanges(server, user, channel, tokens);
}

bool Command::handleModeChanges(Server* server, User* user, Channel* channel, const std::vector<std::string>& tokens)
{
    const std::string& modeString = tokens[2];
    size_t paramIndex = 3;
    bool adding = true;
    std::string appliedModes = "";
    std::string modeParams = "";
    
    for (size_t i = 0; i < modeString.length(); ++i)
    {
        char mode = modeString[i];
        
        if (mode == '+')
        {
            adding = true;
            continue;
        }
        else if (mode == '-')
        {
            adding = false;
            continue;
        }
        
        switch (mode)
        {
            case 'i': // Invite-only
                channel->set_invite_only(adding);
                appliedModes += (adding ? "+" : "-");
                appliedModes += "i";
                logUserAction(user->getNickname(), user->getFd(), 
                    (adding ? "enabled" : "disabled") + std::string(" invite-only for ") + channel->get_name());
                break;
                
            case 't': // Topic protection
                channel->set_topic_protection(adding);
                appliedModes += (adding ? "+" : "-");
                appliedModes += "t";
                logUserAction(user->getNickname(), user->getFd(), 
                    (adding ? "enabled" : "disabled") + std::string(" topic protection for ") + channel->get_name());
                break;
                
            case 'l': // User limit
                if (adding)
                {
                    if (paramIndex < tokens.size())
                    {
                        int limit = std::atoi(tokens[paramIndex].c_str());
                        if (limit > 0)
                        {
                            channel->set_user_limit(limit);
                            appliedModes += "+l";
                            modeParams += " " + toString(limit);
                            logUserAction(user->getNickname(), user->getFd(), 
                                "set user limit to " + toString(limit) + " for " + channel->get_name());
                        }
                        paramIndex++;
                    }
                }
                else
                {
                    channel->set_user_limit(0);
                    appliedModes += "-l";
                    logUserAction(user->getNickname(), user->getFd(), 
                        "removed user limit for " + channel->get_name());
                }
                break;
                
            case 'k': // Channel key
                if (adding)
                {
                    if (paramIndex < tokens.size())
                    {
                        const std::string& key = tokens[paramIndex];
                        channel->set_password(key);
                        appliedModes += "+k";
                        modeParams += " " + key;
                        logUserAction(user->getNickname(), user->getFd(), 
                            "set channel key for " + channel->get_name());
                        paramIndex++;
                    }
                }
                else
                {
                    channel->set_password("");
                    appliedModes += "-k";
                    logUserAction(user->getNickname(), user->getFd(), 
                        "removed channel key for " + channel->get_name());
                }
                break;
                
            case 'o': // Operator
                if (paramIndex < tokens.size())
                {
                    const std::string& targetNick = tokens[paramIndex];
                    User* targetUser = server->getUser(targetNick);
                    
                    if (!targetUser)
                    {
                        user->replyError(401, targetNick, "No such nick/channel");
                        paramIndex++;
                        continue;
                    }
                    
                    if (!channel->is_user_member(targetNick))
                    {
                        user->replyError(441, targetNick + " " + channel->get_name(), "They aren't on that channel");
                        paramIndex++;
                        continue;
                    }
                    
                    if (adding)
                    {
                        channel->make_user_operator(targetNick);
                        appliedModes += "+o";
                        modeParams += " " + targetNick;
                        logUserAction(user->getNickname(), user->getFd(), 
                            "gave operator status to " + targetNick + " in " + channel->get_name());
                    }
                    else
                    {
                        channel->remove_user_operator_status(targetNick);
                        appliedModes += "-o";
                        modeParams += " " + targetNick;
                        logUserAction(user->getNickname(), user->getFd(), 
                            "removed operator status from " + targetNick + " in " + channel->get_name());
                    }
                    paramIndex++;
                }
                break;
                
            default:
                user->replyError(472, std::string(1, mode), "is unknown mode char to me");
                continue;
        }
    }
    
    // Broadcast mode changes to channel members if any modes were applied
    if (!appliedModes.empty())
    {
        std::string modeMessage = ":" + user->getNickname() + " MODE " + channel->get_name() 
                                 + " " + appliedModes + modeParams + "\r\n";
        broadcastToChannel(server, channel, modeMessage);
    }
    
    return true;
}
