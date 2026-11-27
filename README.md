# Toronto-Geocoding

A geocoder takes a street address such as 123 MAIN ST and determines a numerical positional representation of it, for example coordinates (longitude, latitude). This geocoder determines the coordinate of a valid address in the City of Toronto by approximation, using validated and existing data points provided by the city. Having datasets with data points that are labeled with location data opens up the data for more forms of data analysis (e.g. regional trends, geographic distribution of data, etc.) which wouldnt be possible without it. 

This geocoder relies on two datasets which are provided by the City of Toronto and are listed below:
- Address Points (Municipal) - Toronto One Address Repository
    - Contains over 500,000 addresses found within the City of Toronto
    - Each address point is described with a series of attributes including street number, street name, address type, feature class and real world coordinates.
    - https://open.toronto.ca/dataset/address-points-municipal-toronto-one-address-repository/
- Toronto Centreline (TCL)
    - A dataset where each entry represents streets/roads, walkways, rivers, railways, and highways.
    - Each line segment is described with a series of attributes including a unique identifier, name, feature code, and address ranges.
    - https://open.toronto.ca/dataset/toronto-centreline-tcl/

This geocoder works by inerpolation using the existing address to coordinate maps provided by the Address Points and Centreline datasets. 