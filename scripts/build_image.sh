
# Builds the Docker image every build runs in (vasal-build), from
# .docker/dockerfile. Run it once, and again after the dockerfile changes.
# Run it from the scripts folder.
#
#	bash build_image.sh

cd .. || exit 1
exec docker build --tag vasal-build - < .docker/dockerfile
