import React from "react";
import Plot from "react-plotly.js";

export type ServerData = {
  time: number;
  x_x: number;
  x_y: number;
  building: string;
};

export type ServerDataList = ServerData[];

export type ServerResponse = {
  data: ServerDataList;
};

export function useData() {
  let emptyData: ServerDataList = [{ time: 0, x_x: 0, x_y: 0, building: "" }];

  const [data, setData] = React.useState(emptyData);

  async function fetchData() {
    let response = await fetch(
      "http://608dev-2.net/sandbox/sc/team10/server.py?wherehaveibeen=1"
    );
    let json: ServerResponse = await response.json();
    setData(json["data"]);
  }

  type Coord = google.maps.LatLngLiteral;

  function formatData(data: ServerDataList) {
    const result: Array<Coord> = [];
    const dataLength = data.length;
    if (dataLength === 1) {
      result.push({
        lat: data[0]["x_y"],
        lng: data[0]["x_x"],
      });
      return result;
    }

    let maxLocs = 20;
    for (let i = dataLength - 1; i > dataLength - maxLocs - 1; i = i - 1) {
      result.push({
        lng: data[i]["x_x"],
        lat: data[i]["x_y"],
      });
    }
    return result;
  }

  React.useEffect(() => {
    const interval = setInterval(() => {
      console.log("running");
      fetchData();
    }, 1000);
    return () => clearInterval(interval);
  });

  fetchData();

  return formatData(data);
}

export function RobotPlots(data: ServerDataList) {
  return (
    <div className="p-10">
      <div className="text-xl mb-5 font-bold underline text-slate-800">
        Data
      </div>
      <p className="text-lg">
        {data && (
          <div className="flex flex-row flex-wrap">
            {/* <Plot
              data={[
                {
                  x: data["times"],
                  y: data["x_x"],
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
              layout={{ width: 320, height: 240, title: "X Position" }}
            /> */}
            {/* <Plot
              data={[
                {
                  x: data["times"],
                  y: data["x_y"],
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
            layout={{ width: 320, height: 240, title: "Y Position" }}
            /> */}
          </div>
        )}
      </p>
    </div>
  );
}

export default RobotPlots;
