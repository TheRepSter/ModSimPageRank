#include <zim/archive.h>
#include <zim/error.h>
#include <fstream>
#include <iostream>
#include <string>


/*
    Programa que extreu del .zim de la Viquipedia tots els articles que volem per el nostre subgraf.
*/

// Llista de ciutats del Vallès.
const std::string ciutats[] = {
    // Vallès Occidental (23 municipis)
    "Badia del Vallès",
    "Barberà del Vallès",
    "Castellar del Vallès",
    "Castellbisbal",
    "Cerdanyola del Vallès",
    "Gallifa",
    "Matadepera",
    "Montcada i Reixac",
    "Palau-solità i Plegamans",
    "Polinyà",
    "Rellinars",
    "Ripollet",
    "Rubí",
    "Sabadell",
    "Sant Cugat del Vallès",
    "Sant Llorenç Savall",
    "Sant Quirze del Vallès",
    "Santa Perpètua de Mogoda",
    "Sentmenat",
    "Terrassa",
    "Ullastrell",
    "Vacarisses",
    "Viladecavalls",

    // Vallès Oriental (38 municipis)
    "Bigues i Riells",
    "Caldes de Montbui",
    "Campins",
    "Canovelles",
    "Cànoves i Samalús",
    "Cardedeu",
    "Fogars de Montclús",
    "Figaró-Montmany",
    "Gualba",
    "Granollers",
    "La Garriga",
    "La Llagosta",
    "La Roca del Vallès",
    "Les Franqueses del Vallès",
    "Lliçà d'Amunt",
    "Lliçà de Vall",
    "Llinars del Vallès",
    "L'Ametlla del Vallès",
    "Martorelles",
    "Mollet del Vallès",
    "Montmeló",
    "Montornès del Vallès",
    "Montseny",
    "Parets del Vallès",
    "Sant Antoni de Vilamajor",
    "Sant Celoni",
    "Sant Esteve de Palautordera",
    "Sant Feliu de Codines",
    "Sant Fost de Campsentelles",
    "Sant Pere de Vilamajor",
    "Santa Eulàlia de Ronçana",
    "Santa Maria de Martorelles",
    "Santa Maria de Palautordera",
    "Tagamanent",
    "Vallgorguina",
    "Vallromanes",
    "Vilanova del Vallès",
    "Vilalba Sasserra"
};


int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Us: " << argv[0] << " <file.zim>" << std::endl;
        return 1;
    }
    zim::Archive archive(argv[1]);
    std::ofstream nameToId("nameToId.txt");

    for (const auto ciutat : ciutats) {
        try {
            auto entry = archive.getEntryByTitle(ciutat);
            if (entry.isRedirect()) {
                entry = entry.getRedirectEntry();
            }
            nameToId << ciutat << " :: " << entry.getIndex() << std::endl;
        } catch (const zim::EntryNotFound &e) {
            std::cout << "No s'ha trobat " << ciutat << "!" << std::endl;
            return 1;
        }
    }
}