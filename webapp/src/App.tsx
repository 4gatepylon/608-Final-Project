import * as React from "react";
import * as ReactDom from "react-dom";
import { Wrapper, Status } from "@googlemaps/react-wrapper";
import { useState } from "react";
import "./App.css";
import { Map, Marker, Polyline } from "./components/Map";
import { useData } from "./components/RobotPlots";
import { ValuesAndNavBar } from "./components/NavBar";
import Camera from "./components/Camera";
import PathHandler from "./components/PathHandler";

console.log(process.env);

// TODOs for the future:
// - Add a buttom to make a path across MIT using the time
// - update the logo of the markers used.
// can you push this and then update the server?

type Location = google.maps.LatLngLiteral;

const App = () => {
  const MIT_Coords = { lat: 42.3595447562244, lng: -71.09189772577619 };
  const MIT_Zoom = 16;

  // state for whether we are on map
  const [showMap, setShowMap] = React.useState(false);
  // state for whether we are on camera
  const [showCamera, setShowCamera] = React.useState(false);
  // state for whether we are on path handler
  const [showPathHandler, setShowPathHandler] = React.useState(true);

  // This sets up the initial position of the map.
  const [clicks, setClicks] = useState<google.maps.LatLng[]>([]);
  const [center, setCenter] = useState<google.maps.LatLngLiteral>(MIT_Coords);
  const [zoom, setZoom] = useState(MIT_Zoom);

  // use the Data from the server
  const locationList = useData();

  // setup the state for autorefresh
  const [autoRefresh, setAutoRefresh] = useState(false);

  const onClick = (e: google.maps.MapMouseEvent) => {
    setClicks([...clicks, e.latLng]);
  };

  const onIdle = (m: google.maps.Map) => {
    console.log("onIdle");
    setZoom(m.getZoom()!);
    setCenter(m.getCenter()!.toJSON());
  };

  let apiKey = process.env.REACT_APP_GOOGLE_MAPS_API_KEY;
  if (!apiKey) {
    apiKey = "";
  }

  const render = (status: Status) => {
    return <h1>{status}</h1>;
  };

  const mapHtml = (
    <div className="h-auto absolute top-0 left-0 right-0 bottom-0 mt-14">
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

  const cameraHtml = <Camera />;

  const pathSaver = <PathHandler />;

  return (
    <div>
      <ValuesAndNavBar
        setShowMap={(x: boolean) => setShowMap(x)}
        setShowCamera={(x: boolean) => setShowCamera(x)}
        setShowPathHandler={(x: boolean) => setShowPathHandler(x)}
      />
      {showMap && mapHtml}
      {showCamera && cameraHtml}
      {showPathHandler && pathSaver}
    </div>
  );
};

window.addEventListener("DOMContentLoaded", () => {
  ReactDom.render(<App />, document.getElementById("root"));
});

export default App;
