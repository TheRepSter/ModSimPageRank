#include <fstream>
#include <iostream>
#include <string>
#include <zim/archive.h>
#include <zim/entry.h>
#include <zim/item.h>
#include <zim/error.h>
#include <unordered_set>
#include <vector>

inline int ishex(int x) {
    return (x >= '0' && x <= '9') ||
           (x >= 'a' && x <= 'f') ||
           (x >= 'A' && x <= 'F');
}

int decode(const char *s, char *dec) {
    char *o;
    const char *end = s + strlen(s);
    int c;
    for (o = dec; s <= end; o++) {
        c = *s++;
        if (c == '+')
            c = ' ';
        else if (c == '%') {
            /* URLs mal fetes */
            if (s + 2 <= end && ishex(s[0]) && ishex(s[1]) &&
                sscanf(s, "%2x", &c) == 1)
                s += 2;
        }

        if (dec)
            *o = c;
    }
    return o - dec;
}

std::string sanitize(std::string data) {
    size_t hash_pos = data.find('#');
    if (hash_pos != std::string::npos) {
        data = data.substr(0, hash_pos);
    }
    char *dec = (char*)malloc(sizeof(char)*(data.size() + 1));
    if (dec == NULL) {
        throw std::runtime_error("No hi ha espai...");
    }
    decode(data.c_str(), dec);
    std::string result(dec);
    free(dec);
    result.erase(0, result.find_first_not_of(" \t\n\r|"));
    result.erase(result.find_last_not_of(" \t\n\r|") + 1);
    return result;
}

// Eliminem <tag openMarker...>...</tag> de les dades.
void removeTagBlocks(std::string &data,
                     const std::string &tag,
                     const std::string &openMarker,
                     const std::string &path) {
    const std::string openToken  = "<"  + tag;
    const std::string closeToken = "</" + tag + ">";
    size_t search_from = 0;
    while (true) {
        size_t block_start = data.find(openToken, search_from);
        if (block_start == std::string::npos) break;

        size_t tag_end = data.find(">", block_start);
        if (tag_end == std::string::npos) break;

        std::string opening_tag = data.substr(block_start, tag_end - block_start + 1);
        if (opening_tag.find(openMarker) == std::string::npos) {
            search_from = tag_end + 1;
            continue;
        }

        int depth = 1;
        size_t cursor = tag_end + 1;
        size_t block_end = std::string::npos;
        bool malformed = false;
        while (depth > 0) {
            size_t next_open  = data.find(openToken, cursor);
            size_t next_close = data.find(closeToken, cursor);
            if (next_close == std::string::npos) {
                std::cerr << "No es pot tancar " << closeToken << " en " << path << std::endl;
                malformed = true; break;
            }
            if (next_open != std::string::npos && next_open < next_close) {
                depth++;
                cursor = next_open + openToken.size();
            } else {
                depth--;
                cursor = next_close + closeToken.size();
                if (depth == 0) block_end = cursor;
            }
        }
        if (malformed) { search_from = tag_end + 1; continue; }
        data.erase(block_start, block_end - block_start);
    }
}

// retorna la posició del primer final del article (és a dir, referencies o bibliografia)
size_t findCutoff(const std::string &data) {
    size_t cutoff = std::string::npos;
    auto consider = [&](size_t p) {
        if (p != std::string::npos && (cutoff == std::string::npos || p < cutoff))
            cutoff = p;
    };

    size_t search_from = 0;
    while (true) {
        size_t sp = data.find("<section data-mw-section-id=", search_from);
        if (sp == std::string::npos) break;
        size_t se = data.find(">", sp);
        if (se == std::string::npos) break;
        std::string so = data.substr(sp, se - sp + 1);
        if (so.find("aria-labelledby=\"Refer\xC3\xA8ncies\"") != std::string::npos
            || so.find("aria-labelledby=\"Referencias\"") != std::string::npos
            || so.find("aria-labelledby=\"Bibliografia\"") != std::string::npos)
            consider(sp);
        search_from = se + 1;
    }
    consider(data.find("<h2 id=\"Refer\xC3\xA8ncies\""));
    consider(data.find("<h2 id=\"Bibliografia\""));
    consider(data.find(">Refer\xC3\xA8ncies</h2>"));
    consider(data.find(">Bibliografia</h2>"));
    return cutoff;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Us: " << argv[0] << " <file.zim>" << std::endl;
        return 1;
    }

    std::ofstream log_file("log.txt");

    std::unordered_set<zim::entry_index_type> ids_done;

    try {
        zim::Archive archive(argv[1]);
        std::ofstream file("entriesConnections.txt");

        for (const auto &entry : archive.iterByPath()) {
            zim::Entry handler = entry;
            log_file << "Procesant entrada " << handler.getPath() << std::endl;

            if (handler.isRedirect()) handler = handler.getRedirectEntry();

            zim::Item item = handler.getItem();
            zim::entry_index_type index = item.getIndex();
            if (ids_done.find(index) != ids_done.end()) continue;
            ids_done.insert(index);

            if (handler.getPath().rfind("Portada/", 0) == 0) continue;

            std::vector<zim::entry_index_type> articles_menctioned;
            std::unordered_set<zim::entry_index_type> articles_menctioned_seen;

            std::string mimetype = item.getMimetype();
            if (mimetype == "text/html" || mimetype == "text/html; charset=iso-8859-1") {
                std::string data = item.getData();

                // 1. Només nececitem <body>
                size_t body_start = data.find("<body");
                if (body_start == std::string::npos) { data = ""; goto write_entry; }
                data = data.substr(body_start);
                {
                    size_t body_end = data.find("</body");
                    if (body_end != std::string::npos) data = data.substr(0, body_end + 7);
                }

                // 2. Eliminar taules (infoboxes, navboxes, caixes de taxonomia, etc.).
                removeTagBlocks(data, "table", "", handler.getPath());

                // 3. Eliminar navboxes que queden.
                removeTagBlocks(data, "div", "class=\"navbox",  handler.getPath());
                removeTagBlocks(data, "div", "role=\"navigation\" class=\"navbox", handler.getPath());

                // 4. Eliminar el final footer.
                {
                    size_t footer = data.find("This article is issued from");
                    if (footer != std::string::npos) data = data.substr(0, footer);
                }

                // 5. Eliminar fins Referencies / Bibliografia.
                {
                    size_t cutoff = findCutoff(data);
                    if (cutoff != std::string::npos) data = data.substr(0, cutoff);
                }

                // 6. Quedarse només amb el contingut dintre <p> i <li>.
                {
                    const std::pair<std::string, std::string> tags[] = {
                        {"<p",  "</p>"},
                        {"<li", "</li>"},
                    };
                    std::string collected;
                    for (const auto &[open, close] : tags) {
                        size_t search_from = 0;
                        while (true) {
                            size_t block_open = data.find(open, search_from);
                            if (block_open == std::string::npos) break;
                            char next = data[block_open + open.size()];
                            if (next != '>' && next != ' ' && next != '\n' && next != '\t') {
                                search_from = block_open + 1;
                                continue;
                            }
                            size_t tag_end = data.find(">", block_open);
                            if (tag_end == std::string::npos) break;
                            size_t block_close = data.find(close, tag_end);
                            if (block_close == std::string::npos) {
                                search_from = tag_end + 1;
                                continue;
                            }
                            collected += data.substr(tag_end + 1, block_close - tag_end - 1);
                            collected += ' ';
                            search_from = block_close + close.size();
                        }
                    }
                    data = std::move(collected);
                }

                while (data.find("href=\"") != std::string::npos) {
                    size_t pos = data.find("href=\"");
                    size_t a_pos = data.rfind("<a ", pos);
                    if (a_pos == std::string::npos) {
                        std::string path = handler.getPath();
                        size_t slash_pos = path.find("/");
                        if (slash_pos != std::string::npos) path.replace(slash_pos, 1, "-");
                        std::ofstream html_file(path + ".html");
                        html_file << item.getData();
                        html_file.close();
                        std::cout << "Eliminant href per que no és correcta. Revisar a " << path + ".html" << std::endl;
                        data = data.substr(pos + 6);
                        continue;
                    }
                    {
                        size_t a_tag_end = data.find(">", a_pos);
                        std::string a_tag = data.substr(a_pos, a_tag_end != std::string::npos ? a_tag_end - a_pos : 0);
                        if (a_tag.find("mw-selflink") != std::string::npos) {
                            data = data.substr(pos + 6);
                            continue;
                        }
                    }
                    std::string href = data.substr(pos + 6, data.find("\"", pos + 6) - pos - 6);
                    size_t size_href = href.size();
                    href = sanitize(href);
                    while (href.rfind("../", 0) == 0) href = href.substr(3);
                    while (href.rfind("./", 0) == 0)  href = href.substr(2);
                    std::string href_lower = href;
                    std::transform(href_lower.begin(), href_lower.end(), href_lower.begin(), ::tolower);

                    if (href_lower.rfind("/profiles/", 0) == 0) {
                        std::cout << href << " es un perfil, pasant" << std::endl;
                        data = data.substr(pos + size_href);
                        continue;
                    }
                    // URLs que no porten enlloc
                    if (href_lower.rfind("http://", 0) == 0
                        || href_lower.rfind("https://", 0) == 0
                        || href_lower.empty()
                        || href_lower.rfind("geo:", 0) == 0
                        || href_lower.rfind("news:", 0) == 0
                        || href_lower.rfind("mailto:", 0) == 0
                        || href_lower.rfind("ftp:", 0) == 0
                        || href_lower.rfind("tel:", 0) == 0
                        || href_lower.rfind("irc:", 0) == 0
                        || href_lower.rfind("urn:", 0) == 0
                        || href_lower.rfind("vp:", 0) == 0
                        || href_lower.rfind("{{format ref}}", 0) == 0 // Article sobre HTML. 
                        || href_lower == "enlace" // Article sobre HTML, aixo ho trenca
                        || href_lower.rfind("categoria:esglésies_de_l'horta_nord", 0) == 0 // Aquest link no existeix
                        || href_lower.rfind("git:", 0) == 0
                        || href_lower.rfind("<a class=", 0) == 0
                        || href_lower.rfind("pictures/", 0) == 0
                        || href_lower.rfind("categoria:patrimoni_monumental_del_gironès", 0) == 0 // Aquest link no existeix
                        || href_lower.rfind("ircs:", 0) == 0
                        || href_lower.rfind("svn:", 0) == 0
                        || href_lower.rfind("magnet:", 0) == 0
                        || href_lower.rfind("mms:", 0) == 0
                    ) {
                        if (href.find("https://ca.wikipedia.org/wiki/") != std::string::npos || href.find("http://ca.wikipedia.org/wiki/") != std::string::npos) {
                            if (href.find("ca.wikipedia.org") > href.find("?")) {
                                data = data.substr(pos + size_href);
                                continue;
                            }
                            if (href.find("?title=") != std::string::npos) {
                                try {
                                    href = href.substr(href.find("?title=") + 7);
                                    href = href.substr(0, href.find("&amp;"));
                                    if (href.rfind("VP:", 0) == 0 
                                        || href.empty() 
                                        || href.rfind("Plantilla:", 0) == 0
                                        || href.rfind("Viquiprojecte:", 0) != std::string::npos
                                        || href.rfind("Especial:", 0) != std::string::npos
                                        || href.rfind("Template:", 0) != std::string::npos
                                        || href.rfind("Viquipèdia:Autoritzacions", 0) != std::string::npos
                                    ) {
                                        data = data.substr(pos + size_href);
                                        continue;
                                    }
                                    zim::Entry new_entry = archive.getEntryByPath(href);
                                    if (new_entry.isRedirect()) new_entry = new_entry.getRedirectEntry();
                                    if (articles_menctioned_seen.insert(new_entry.getIndex()).second)
                                        articles_menctioned.push_back(new_entry.getIndex());
                                    data = data.substr(pos + size_href);
                                    continue;
                                } catch (const zim::EntryNotFound &) {
                                    std::cout << "Entry not found for ?title= url " << href << ", skipping" << std::endl;
                                }
                            }
                            if (href.rfind("https://ca.wikipedia.org/wiki/", 0) == 0) {
                                try {
                                    std::string wiki_path = href.substr(strlen("https://ca.wikipedia.org/wiki/"));
                                    if (wiki_path.find("redlink=1") != std::string::npos) {
                                        data = data.substr(pos + size_href);
                                        continue;
                                    }
                                    if (wiki_path.rfind("VP:", 0) == 0
                                        || wiki_path.empty()
                                        || wiki_path.rfind("Plantilla:", 0) == 0
                                        || wiki_path.rfind("Viquiprojecte:", 0) != std::string::npos
                                        || wiki_path.rfind("Especial:", 0) != std::string::npos
                                        || wiki_path.rfind("Template:", 0) != std::string::npos
                                        || wiki_path.rfind("Categoria:Premis_de_traducció", 0) != std::string::npos
                                        || wiki_path.rfind("Viquipèdia:Autortizacions", 0) != std::string::npos
                                        || wiki_path.rfind("Portada", 0) == 0
                                    ) {
                                        data = data.substr(pos + size_href);
                                        continue;
                                    }
                                    zim::Entry new_entry = archive.getEntryByPath(wiki_path);
                                    if (new_entry.isRedirect()) new_entry = new_entry.getRedirectEntry();
                                    if (articles_menctioned_seen.insert(new_entry.getIndex()).second)
                                        articles_menctioned.push_back(new_entry.getIndex());
                                    data = data.substr(pos + size_href);
                                    continue;
                                } catch (const zim::EntryNotFound &) {
                                    std::cout << "No s'ha trobat l'article pel link " << href << ", passant" << std::endl; // Teoricament no surt
                                }
                            }
                            std::string path = handler.getPath();
                            size_t slash_pos = path.find("/");
                            if (slash_pos != std::string::npos) path.replace(slash_pos, 1, "-");
                            std::ofstream html_file(path + ".html");
                            html_file << item.getData();
                            html_file.close();
                            std::cout << href << " no trobat, guardant a l'archiu per revisar-ho manualment: " << path + ".html" << std::endl;
                        }
                        data = data.substr(pos + size_href);
                        continue;
                    }
                    try {
                        zim::Entry new_entry = archive.getEntryByPath(href);
                        if (new_entry.isRedirect()) new_entry = new_entry.getRedirectEntry();
                        if (articles_menctioned_seen.insert(new_entry.getIndex()).second)
                            articles_menctioned.push_back(new_entry.getIndex());
                    } catch (const zim::EntryNotFound &) {
                        std::string path = handler.getPath();
                        size_t slash_pos = path.find("/");
                        if (slash_pos != std::string::npos) path.replace(slash_pos, 1, "-");
                        std::ofstream html_file(path + ".html");
                        html_file << item.getData();
                        html_file.close();
                        std::cout << href << " revisa-ho aqui: " << path + ".html" << std::endl;
                        throw std::runtime_error(href + " no trobat");
                    }
                    data = data.substr(pos + size_href);
                }

                write_entry:
                file << index << " :: ";
                for (const auto &article : articles_menctioned)
                    file << article << " ";
                file << std::endl;
            }
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
