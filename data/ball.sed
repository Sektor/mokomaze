# Smaller image to prevent aliasing
s/width="620"/width="128"/
s/height="620"/height="128"/

# Hide the putter
/inkscape:label="putter"/ {
	n
	s/display:inline/display:none/
}

# Show the ball shadow and scale it down to prevent aliasing
/inkscape:label="ballshadow"/ {
	s/$/ transform="scale(0.20645161290322581)"/
	n
	s/display:none/display:inline/
}

# Scale the ball down a bit to prevent aliasing
/inkscape:label="ball"/ {
	s/$/ transform="scale(0.20645161290322581)"/
}

# Change the red parts to white
/id="path11852"/ {
	n
	s/fill:#ff0000/fill:#ffffff/
}

# Change the black/grey parts to brown
/id="rect12958"/ {
	n
	n
	s/fill:#000000/fill:#953c00/
}
/id="path12908"/ {
	n
	s/fill:#000000/fill:#953c00/
}
