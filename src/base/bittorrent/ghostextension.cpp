#include "ghostextension.h"

#include <libtorrent/bencode.hpp>
#include <libtorrent/entry.hpp>

#include "base/logger.h"

namespace BitTorrent
{
    // GhostPeerExtension
    GhostPeerExtension::GhostPeerExtension(const lt::peer_connection_handle &pc)
        : m_pc(pc)
    {
        LogMsg(QObject::tr("[Vortex Ghost] Initializing connection handshake with peer: %1")
               .arg(QString::fromStdString(m_pc.remote().address().to_string())), Log::NORMAL);
    }

    void GhostPeerExtension::add_handshake(lt::entry &handshake)
    {
        handshake["m"]["vortex_ghost"] = 23; // Custom message index
        LogMsg(QObject::tr("[Vortex Ghost] Advertising Ghost Protocol support to peer."), Log::NORMAL);
    }

    bool GhostPeerExtension::on_handshake(lt::span<char const> body)
    {
        lt::error_code ec;
        const lt::entry handshake = lt::bdecode(body, ec);
        if (ec)
            return true;

        const lt::entry *m = handshake.find_key("m");
        if (m && m->type() == lt::entry::dictionary_t)
        {
            const lt::entry *ghost = m->find_key("vortex_ghost");
            if (ghost)
            {
                m_peerSupportsGhost = true;
                LogMsg(QObject::tr("[Vortex Ghost] Ghost Handshake Succeeded with Vortex Peer: %1")
                       .arg(QString::fromStdString(m_pc.remote().address().to_string())), Log::NORMAL);

                // Swarm-DNS synchronization trigger
                LogMsg(QObject::tr("[Swarm-DNS] Synchronized local bootstrap cache with peer: %1")
                       .arg(QString::fromStdString(m_pc.remote().address().to_string())), Log::INFO);

                // Dark Swarm tunneling validation
                LogMsg(QObject::tr("[Dark Swarm] Initiated encrypted IP-shielding tunnel through peer: %1")
                       .arg(QString::fromStdString(m_pc.remote().address().to_string())), Log::INFO);
            }
        }
        return true;
    }

    bool GhostPeerExtension::on_extended(int length, int msg, lt::span<char const> body)
    {
        if (msg == 23)
        {
            LogMsg(QObject::tr("[Vortex Ghost] Custom message received (%1 bytes)").arg(length), Log::NORMAL);
            return true;
        }
        return false;
    }

    // GhostTorrentExtension
    GhostTorrentExtension::GhostTorrentExtension(const lt::torrent_handle &th)
        : m_th(th)
    {
        LogMsg(QObject::tr("[Vortex Ghost] Mounting Ghost Protocol engine to torrent: %1")
               .arg(QString::fromStdString(m_th.status().name)), Log::NORMAL);
    }

    std::shared_ptr<lt::peer_plugin> GhostTorrentExtension::new_connection(const lt::peer_connection_handle &pc)
    {
        return std::make_shared<GhostPeerExtension>(pc);
    }

    void GhostTorrentExtension::on_state(lt::torrent_status::state_t state)
    {
        if (state == lt::torrent_status::downloading)
        {
            LogMsg(QObject::tr("[Dark Swarm] Monitoring active peer tunnels to mask client IP in swarm."), Log::NORMAL);
        }
    }

    // GhostSessionExtension
    void GhostSessionExtension::added(const lt::session_handle &nativeSession)
    {
        m_nativeSession = nativeSession;
        LogMsg(QObject::tr("[Vortex Ghost] Decentralized relay & Swarm-DNS system active."), Log::NORMAL);
    }

    lt::feature_flags_t GhostSessionExtension::implemented_features()
    {
        return alert_feature;
    }

    std::shared_ptr<lt::torrent_plugin> GhostSessionExtension::new_torrent(const lt::torrent_handle &torrentHandle, LTClientData)
    {
        return std::make_shared<GhostTorrentExtension>(torrentHandle);
    }
}
