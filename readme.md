# Motivació

Històricament, el Vallès ha tingut múltiples capitals, ara mateix sent Terrassa, Sabadell i Granollers, i sempre hi ha hagut el debat de quina és la ciutat més important del Vallès. Com es podria mesurar quina és? PageRank serveix precisament per això, dissenyat per veure quines són les pàgines web més importants. Si canviem pàgina web per ciutat, tindrem la resposta i finalment podrem tancar el debat històric.

# Metodologia

Hem fet dues implementacions: 
1. PageRank amb una xarxa composta únicament amb els nodes de les ciutats a estudiar, les arestes sent els enllaços que transporten d'un node a un altre. És el pageRank.ipynb
2. PageRank amb tota la Viquipèdia. Es troba tot a allPageRank programat amb c++. Això no obstant, la majoria documentació, resultats i el seu analisi es poden seguir al pageRank.ipynb. Per executar certs programes és necessari un .zim de la Viquipèdia (wikipedia_ca_all_nopic) que es pot descarregar [aquí](https://download.kiwix.org/zim/wikipedia/) i libzim. Està fet amb iteracions en canvi d'autovectors, ja que són 700.000 nodes, i una matriu 700 mil x 700 mil és inviable. Hem aplicat l'[algoritme](https://en.wikipedia.org/wiki/PageRank#Iterative) en c++.

# Transparència amb l'ús de la IA:  

- Per la part de PageRank amb tota la Viquipèdia no hem fet servir la IA. Part del codi és reutilitzat d'un altre projecte personal d'un membre del grup entusiasta amb la Viquipèdia.
- Per la part de el notebook per el cas específic del vallès, hem fet ús de la IA generativa "kimi" de moonshot i "claude" de anthropic; a mode de respondre el tipus de dubtes que fa un parell d'anys hauriem resolt mitjançant eines com stack overflow/foros, buscant una explicació teòrica de què ens falla i no entenem, en cap cas ens ha escrit ninguna part de codi. Aixi facilitant-nos tant el entendre codi que hagin fet companys nostres com resolvent errors. Cal remarcar que l'hem fet servir en casos comptats, no ha estat una eina truncal per el desenvolupament del nostre projecte.
Grup H. 

Eduardo Pérez Motato, terrasenc (1709992)
Félix Sáiz von Fraunberg, sabadellenc (1620854)
Benet Carbonell Fusté, terrassenc (1709685)
