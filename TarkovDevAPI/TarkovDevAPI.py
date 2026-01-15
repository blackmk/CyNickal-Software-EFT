import sqlite3
import requests

def run_query(query):
    headers = {"Content-Type": "application/json"}
    response = requests.post('https://api.tarkov.dev/graphql', headers=headers, json={'query': query})
    if response.status_code == 200:
        return response.json()
    else:
        raise Exception("Query failed to run by returning code of {}. {}".format(response.status_code, query))

ItemQuery = """
{
  items(lang: en) {
    id
    sellFor {
      priceRUB
      vendor {
        name
      }
    }
    shortName
  }
}
"""
def UpdateItemTable():
    con = sqlite3.connect('../CyNickal Software EFT/EFT_Data.db');
    cur = con.cursor();
    result = run_query(ItemQuery);
    for Item in result['data']['items']:

        HighestTraderPrice = -1;
            
        for SellOption in Item['sellFor']:
            price = SellOption['priceRUB'];
            vendor = SellOption["vendor"]["name"];
            if(vendor == "Flea Market"):
                continue;

            if(price > HighestTraderPrice):
                HighestTraderPrice = price;

        cur.execute("INSERT OR IGNORE INTO item_data (bsg_id, short_name, trader_price) VALUES (?, ?, ?)", (Item['id'], Item['shortName'], HighestTraderPrice));

    con.commit();
    con.close();
    return;

ContainerQuery = """
{
  lootContainers {
    id
    name
  }
}
"""
def UpdateContainerTable():
    con = sqlite3.connect('../CyNickal Software EFT/EFT_Data.db');
    cur = con.cursor();

    result = run_query(ContainerQuery);

    for Item in result['data']['lootContainers']:
       cur.execute("INSERT OR IGNORE INTO container_data (bsg_id, short_name) VALUES (?, ?)", (Item['id'], Item['name']));

    con.commit();
    con.close();
    return;

AmmoQuery ="""
{
  ammo {
    item {
      id
      shortName
    }
  }
}
"""
def UpdateAmmoTable():
    result = run_query(AmmoQuery);

    con = sqlite3.connect('../CyNickal Software EFT/EFT_Data.db');
    cur = con.cursor();

    for Item in result['data']['ammo']:
         cur.execute("INSERT OR IGNORE INTO ammo_data (bsg_id, short_name) VALUES (?, ?)", (Item['item']['id'], Item['item']['shortName']));

    con.commit();
    con.close();
    return;
UpdateAmmoTable();