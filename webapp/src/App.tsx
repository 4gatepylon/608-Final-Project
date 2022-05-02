import * as React from "react";
import * as ReactDom from "react-dom";
import { Wrapper, Status } from "@googlemaps/react-wrapper";
import { useState, useEffect, useMemo } from "react";
import "./App.css";
import {
  useDeepCompareMemoize,
  useDeepCompareEffectForMaps,
} from "./utils/MapUtils";
import { Map, Marker, Polyline } from "./components/Map";
import { useData, ServerData, RobotPlots } from "./components/RobotPlots";

console.log(process.env);

// TODOs for the future:
// - Add a buttom to make a path across MIT using the time
// - update the logo of the markers used.
// can you push this and then update the server?

type Location = google.maps.LatLngLiteral;

const App = () => {
  const MIT_Coords = { lat: 42.3595447562244, lng: -71.09189772577619 };
  const MIT_Zoom = 16;

  // This sets up the initial position of the map.
  const [clicks, setClicks] = useState<google.maps.LatLng[]>([]);
  const [center, setCenter] = useState<google.maps.LatLngLiteral>(MIT_Coords);
  const [zoom, setZoom] = useState(MIT_Zoom);

  // use the Data from the server
  const rawData = useData();

  // setup the state for autorefresh
  const [autoRefresh, setAutoRefresh] = useState(false);

  const locationList: Location[] = useMemo(() => [], []);

  useMemo(() => {
    for (let i = 0; i < rawData["x_y"].length; i++) {
      locationList.push({
        lat: rawData["x_x"][i],
        lng: rawData["x_y"][i],
      });
    }
  }, [locationList, rawData]);

  // now we set the center to the last location in the data
  useEffect(() => {
    const interval = setInterval(() => {
      if (locationList.length > 0 && autoRefresh) {
        setCenter({
          lat: locationList[locationList.length - 1].lat,
          lng: locationList[locationList.length - 1].lng,
        });
        setZoom(25);
      }
    }, 1000);
    return () => clearInterval(interval);
  }, [locationList, autoRefresh]);

  const onClick = (e: google.maps.MapMouseEvent) => {
    setClicks([...clicks, e.latLng!]);
  };

  const onIdle = (m: google.maps.Map) => {
    console.log("onIdle");
    setZoom(m.getZoom()!);
    setCenter(m.getCenter()!.toJSON());
  };
  console.log(locationList);
  const form = (
    <div
      style={{
        padding: "1rem",
        flexBasis: "250px",
        height: "100%",
        overflow: "auto",
      }}
    >
      <label htmlFor="zoom">Zoom</label>
      <input
        type="number"
        id="zoom"
        name="zoom"
        value={zoom}
        onChange={(event) => setZoom(Number(event.target.value))}
      />
      <br />
      <label htmlFor="lat">Latitude</label>
      <input
        type="number"
        id="lat"
        name="lat"
        value={center.lat}
        onChange={(event) =>
          setCenter({ ...center, lat: Number(event.target.value) })
        }
      />
      <br />
      <label htmlFor="lng">Longitude</label>
      <input
        type="number"
        id="lng"
        name="lng"
        value={center.lng}
        onChange={(event) =>
          setCenter({ ...center, lng: Number(event.target.value) })
        }
      />
      <h3>{clicks.length === 0 ? "Click on map to add markers" : "Clicks"}</h3>
      {clicks.map((latLng, i) => (
        <pre key={i}>{JSON.stringify(latLng.toJSON(), null, 2)}</pre>
      ))}
      <button onClick={() => setClicks([])}>Clear</button>
    </div>
  );

  const apiKey = process.env.REACT_APP_GOOGLE_MAPS_API_KEY;
  if (!apiKey) {
    return <h1>Missing API key</h1>;
  }

  const render = (status: Status) => {
    return <h1>{status}</h1>;
  };

  return (
    <div style={{ display: "flex", height: "100%" }}>
      {/* <div className="bg-white h-20 rounded-lg">
        <img
          src={
            "data:image/gif;base64, R0lGODlhAQABAIAAAP///////yH5BAEKAAEALAAAAAABAAEAAAICTAEAOw=="
          }
          className="object-contain"
          alt={"camera output"}
        />
      </div> */}
      <Wrapper apiKey={apiKey} render={render}>
        <Map
          center={center}
          onClick={onClick}
          onIdle={onIdle}
          zoom={zoom}
          style={{ flexGrow: "1", height: "100%" }}
        >
          {clicks.map((latLng, i) => (
            <Marker key={i} position={latLng} />
          ))}

          {locationList.map((location, i) => (
            <Marker key={i * 1000} position={location} />
          ))}
          <Polyline
            path={locationList}
            geodesic={true}
            strokeColor="#FF0000"
            strokeOpacity={1.0}
            strokeWeight={2}
          />
        </Map>
      </Wrapper>
      {/* Basic form for controlling center and zoom of map. */}
      {/* {form} */}
    </div>
  );
};

window.addEventListener("DOMContentLoaded", () => {
  ReactDom.render(<App />, document.getElementById("root"));
});

export default App;
