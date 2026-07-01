#pragma once

#include <libtorrent/extensions.hpp>
#include <libtorrent/peer_connection_handle.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/session_handle.hpp>

#include <memory>
#include <vector>

#include "extensiondata.h"

namespace BitTorrent
{
    class GhostPeerExtension final : public lt::peer_plugin
    {
    public:
        explicit GhostPeerExtension(const lt::peer_connection_handle &pc);
        ~GhostPeerExtension() override = default;

        // peer_plugin interface
        void add_handshake(lt::entry &handshake) override;
        bool on_handshake(lt::span<char const> body) override;
        bool on_extended(int length, int msg, lt::span<char const> body) override;

    private:
        lt::peer_connection_handle m_pc;
        bool m_peerSupportsGhost = false;
    };

    class GhostTorrentExtension final : public lt::torrent_plugin
    {
    public:
        explicit GhostTorrentExtension(const lt::torrent_handle &th);
        ~GhostTorrentExtension() override = default;

        // torrent_plugin interface
        std::shared_ptr<lt::peer_plugin> new_connection(const lt::peer_connection_handle &pc) override;
        void on_state(lt::torrent_status::state_t state) override;

    private:
        lt::torrent_handle m_th;
    };

    class GhostSessionExtension final : public lt::plugin
    {
    public:
        GhostSessionExtension() = default;
        ~GhostSessionExtension() override = default;

        // plugin interface
        void added(const lt::session_handle &nativeSession) override;
        lt::feature_flags_t implemented_features() override;
        std::shared_ptr<lt::torrent_plugin> new_torrent(const lt::torrent_handle &torrentHandle, LTClientData clientData) override;

    private:
        lt::session_handle m_nativeSession;
    };
}
