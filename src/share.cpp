/**
 * @file share.cpp
 * @brief Classes for manipulating share credentials
 *
 * (c) 2013-2014 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#include "mega/share.h"
#include "logging.h"

namespace mega {
Share::Share(User* u, accesslevel_t a, m_time_t t)
{
    user = u;
    access = a;
    ts = t;
}

void Share::serialize(string* d)
{
    handle uh = user ? user->userhandle : 0;
    char a = (char)access;

    d->append((char*)&uh, sizeof uh);
    d->append((char*)&ts, sizeof ts);
    d->append((char*)&a, 1);
    d->append("", 1);
}

bool Share::unserialize(MegaClient* client, int direction, handle h,
                        const byte* ckey, const char** ptr, const char* end, pnode_t n)
{
    if (*ptr + sizeof(handle) + sizeof(m_time_t) + 2 > end)
    {
        return 0;
    }

//    client->newshares.push_back(new NewShare(h, direction, MemAccess::get<handle>(*ptr),
//                                             (accesslevel_t)(*ptr)[sizeof(handle) + sizeof(m_time_t)],
//                                             MemAccess::get<m_time_t>(*ptr + sizeof(handle)), key));

    // Instead of queuing a `NewShare` object to be merged to the `Node` in a later stage, do it
    // here straight away, so the object is fully rebuilt

    handle peer = MemAccess::get<handle>(*ptr);
    accesslevel_t access = (accesslevel_t)(*ptr)[sizeof(handle) + sizeof(m_time_t)];
    m_time_t ts = MemAccess::get<m_time_t>(*ptr + sizeof(handle));

    if (ckey)
    {
        n->sharekey = new SymmCipher(ckey);
    }

    if (!ISUNDEF(peer))
    {
        if (direction)  // outgoing share
        {
            if (!n->outshares)  // a node may contain several outshares
            {
                n->outshares = new share_map;
            }

            Share** sharep = &((*n->outshares)[peer]);

            // modification of existing share or new share
            if (*sharep)
            {
                (*sharep)->update(access, ts);
            }
            else
            {
                *sharep = new Share(client->finduser(peer, 1), access, ts);
            }
        }
        else    // incoming share
        {
            if (peer)
            {
                if (!client->checkaccess(n, OWNERPRELOGIN))
                {
                    n->inshare = new Share(client->finduser(peer, 1), access, ts);
                    n->inshare->user->sharing.insert(n->nodehandle);
                }
                else
                {
                    LOG_warn << "Invalid inbound share location";
                }
            }
            else
            {
                LOG_warn << "Invalid null peer on inbound share";
            }
        }

    }   // end of 'inline mergenewshares()'

    *ptr += sizeof(handle) + sizeof(m_time_t) + 2;

    return true;
}

void Share::update(accesslevel_t a, m_time_t t)
{
    access = a;
    ts = t;
}

// coutgoing: < 0 - don't authenticate, > 0 - authenticate using handle auth
NewShare::NewShare(handle ch, int coutgoing, handle cpeer, accesslevel_t caccess,
                   m_time_t cts, const byte* ckey, const byte* cauth)
{
    h = ch;
    outgoing = coutgoing;
    peer = cpeer;
    access = caccess;
    ts = cts;

    if (ckey)
    {
        memcpy(key, ckey, sizeof key);
        have_key = 1;
    }
    else
    {
        have_key = 0;
    }

    if ((outgoing > 0) && cauth)
    {
        memcpy(auth, cauth, sizeof auth);
        have_auth = 1;
    }
    else
    {
        have_auth = 0;
    }
}
} // namespace
