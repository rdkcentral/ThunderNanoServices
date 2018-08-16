set (PUBLIC_HEADERS
     Client.h 
    )

install(
	FILES ${PUBLIC_HEADERS}
        DESTINATION include/${NAMESPACE}/compositor
)
