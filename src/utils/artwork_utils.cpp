#include "artwork_utils.hpp"
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <mpd/albumart.h>
#include <mpd/readpicture.h>

namespace Utils {
    extern const unsigned char g_defaultAlbumArtData[];
    extern const size_t g_defaultAlbumArtDataLen;
}

namespace miqumusic {

static std::unordered_map<std::string, std::string> s_artwork_cache;

std::string ArtworkUtils::get_default_artwork_path() {
    static std::string s_default_path = "";
    if (!s_default_path.empty() && std::filesystem::exists(s_default_path)) {
        return s_default_path;
    }

    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    std::filesystem::path art_path = temp_dir / "miqumusic_default_album_art.png";

    if (!std::filesystem::exists(art_path)) {
        std::ofstream ofs(art_path, std::ios::binary);
        if (ofs) {
            ofs.write(reinterpret_cast<const char*>(Utils::g_defaultAlbumArtData),
                      Utils::g_defaultAlbumArtDataLen);
            ofs.close();
        }
    }

    s_default_path = art_path.string();
    return s_default_path;
}

std::string ArtworkUtils::resolve_track_artwork(struct mpd_connection* conn, const std::string& song_uri, const std::string& music_dir) {
    if (song_uri.empty()) return get_default_artwork_path();

    auto it = s_artwork_cache.find(song_uri);
    if (it != s_artwork_cache.end() && std::filesystem::exists(it->second)) {
        return it->second;
    }

    // Check directory of file for cover.jpg, cover.png, folder.jpg, etc.
    if (!music_dir.empty()) {
        std::filesystem::path full_song_path = std::filesystem::path(music_dir) / song_uri;
        if (std::filesystem::exists(full_song_path)) {
            std::filesystem::path parent = full_song_path.parent_path();
            static const std::vector<std::string> art_names = {
                "cover.jpg", "cover.png", "Cover.jpg", "Cover.png",
                "folder.jpg", "folder.png", "Folder.jpg", "Folder.png",
                "album.jpg", "album.png", "front.jpg", "front.png"
            };
            for (const auto& name : art_names) {
                std::filesystem::path candidate = parent / name;
                if (std::filesystem::exists(candidate)) {
                    s_artwork_cache[song_uri] = candidate.string();
                    return candidate.string();
                }
            }
        }
    }

    // Try MPD albumart API
    if (conn) {
        char buffer[8192];
        size_t offset = 0;
        std::vector<char> art_data;

        while (true) {
            int bytes_read = mpd_run_albumart(conn, song_uri.c_str(), offset, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                art_data.insert(art_data.end(), buffer, buffer + bytes_read);
                offset += bytes_read;
            } else {
                break;
            }
        }

        if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
            mpd_connection_clear_error(conn);
        }

        // If no albumart, try readpicture (embedded ID3 / FLAC picture)
        if (art_data.empty()) {
            offset = 0;
            while (true) {
                int bytes_read = mpd_run_readpicture(conn, song_uri.c_str(), offset, buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    art_data.insert(art_data.end(), buffer, buffer + bytes_read);
                    offset += bytes_read;
                } else {
                    break;
                }
            }
            if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
                mpd_connection_clear_error(conn);
            }
        }

        if (!art_data.empty()) {
            // Hash or sanitize uri for filename
            std::hash<std::string> hasher;
            std::string filename = "miqumusic_art_" + std::to_string(hasher(song_uri)) + ".png";
            std::filesystem::path out_path = std::filesystem::temp_directory_path() / filename;

            std::ofstream ofs(out_path, std::ios::binary);
            if (ofs) {
                ofs.write(art_data.data(), art_data.size());
                ofs.close();
                s_artwork_cache[song_uri] = out_path.string();
                return out_path.string();
            }
        }
    }

    return get_default_artwork_path();
}

void ArtworkUtils::clear_cache() {
    s_artwork_cache.clear();
}

} // namespace miqumusic
